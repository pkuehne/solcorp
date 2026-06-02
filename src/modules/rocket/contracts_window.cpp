#include "contracts_window.h"
#include "contract_actions.h"
#include "imgui.h"
#include "launch_detail_window.h"
#include "launch_window.h"
#include "modules/engine/gui.h"
#include "modules/engine/helpers.h"
#include "rocket_module.h"
#include <flecs/addons/cpp/entity.hpp>
#include <flecs/addons/cpp/mixins/query/impl.hpp>
#include <spdlog/spdlog.h>
#include <string>

bool contractMatchesFilter(flecs::entity contractE,
                           const ContractsWindow &state) {
  const auto *contract = contractE.try_get<Contract>();
  if (!contract) {
    return false;
  }

  auto world = contractE.world();
  auto openState = world.lookup("States::Contract::Open");
  auto acceptedState = world.lookup("States::Contract::Accepted");
  auto closedState = world.lookup("States::Contract::Closed");

  // Filter by status
  if (state.statusFilter != ContractFilterStatus::All) {
    switch (state.statusFilter) {
    case ContractFilterStatus::Open:
      if (!contractE.has<ContractCurrentState>(openState))
        return false;
      break;
    case ContractFilterStatus::Accepted:
      if (!contractE.has<ContractCurrentState>(acceptedState))
        return false;
      break;
    case ContractFilterStatus::Closed:
      if (!contractE.has<ContractCurrentState>(closedState))
        return false;
      break;
    case ContractFilterStatus::All:
      break;
    }
  }

  // Filter completed contracts
  if (!state.showCompleted && contractE.has<ContractCurrentState>(closedState)) {
    return false;
  }

  return true;
}

bool acceptButtonDisabled(flecs::entity contractE) {
  return !ContractAcceptAction{contractE}.validate(contractE.world());
}

bool rejectButtonDisabled(flecs::entity contractE) {
  return !ContractRejectAction{contractE}.validate(contractE.world());
}

bool planButtonDisabled(flecs::entity contractE) {
  return !contractE.has<ContractCurrentState>(
      contractE.world().lookup("States::Contract::Accepted"));
}

void acceptContract(flecs::world &world, flecs::entity contractE) {
  ContractAcceptAction{contractE}.execute(world);
}

void rejectContract(flecs::world &world, flecs::entity contractE) {
  ContractRejectAction{contractE}.execute(world);
}

LaunchScheduleAction setupLaunchForPayload(flecs::entity payloadE) {
  auto world = payloadE.world();

  auto draftPlan = LaunchScheduleAction{};
  draftPlan.payloads.push_back(payloadE);

  // Find the contract that has this payload and get the target orbit from it
  world.query_builder().with<ContractPayload>(payloadE).build().each(
      [&draftPlan](flecs::entity contractE) {
        draftPlan.targetOrbit = contractE.target<ContractTargetOrbit>();
      });

  showLaunchWindowAdd(world, draftPlan);

  return draftPlan;
}

void showContractsWindow(flecs::world &world) {
  showWindow(world, "Contracts Window");
}

void drawContractsWindow(flecs::entity winE) {
  auto &state = winE.get_mut<ContractsWindow>();
  auto world = winE.world();

  flecs::query<Contract> contractQuery =
      world.query_builder<Contract>().build();

  // --- Filters ---
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("##StatusFilter", "Status")) {
    if (ImGui::Selectable("All",
                          state.statusFilter == ContractFilterStatus::All)) {
      state.statusFilter = ContractFilterStatus::All;
    }
    if (ImGui::Selectable("Open",
                          state.statusFilter == ContractFilterStatus::Open)) {
      state.statusFilter = ContractFilterStatus::Open;
    }
    if (ImGui::Selectable("Accepted", state.statusFilter ==
                                          ContractFilterStatus::Accepted)) {
      state.statusFilter = ContractFilterStatus::Accepted;
    }
    if (ImGui::Selectable("Closed",
                          state.statusFilter == ContractFilterStatus::Closed)) {
      state.statusFilter = ContractFilterStatus::Closed;
    }
    ImGui::EndCombo();
  }

  ImGui::SameLine();
  ImGui::Checkbox("Show Completed", &state.showCompleted);

  ImGui::Separator();

  // --- Table ---
  ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                               ImGuiTableFlags_SizingStretchProp |
                               ImGuiTableFlags_ScrollY;
  if (ImGui::BeginTable("contracts", 10, tableFlags)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Client");
    ImGui::TableSetupColumn("Description");
    ImGui::TableSetupColumn("Status");
    ImGui::TableSetupColumn("Total Payment", ImGuiTableColumnFlags_WidthFixed,
                            100.0f);
    ImGui::TableSetupColumn("Target Orbit", ImGuiTableColumnFlags_WidthFixed,
                            100.0f);
    ImGui::TableSetupColumn("Failed", ImGuiTableColumnFlags_WidthFixed, 45.0f);

    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed,
                            180.0f);
    ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed, 50.0f);
    ImGui::TableHeadersRow();

    bool openDeleteModal = false;
    contractQuery.each([&](flecs::entity contractE, Contract &contract) {
      if (!contractMatchesFilter(contractE, state)) {
        return;
      }

      // Check if this contract has a launch plan
      flecs::entity payloadE = flecs::entity::null();
      flecs::entity launchPlanE = flecs::entity::null();
      world.query_builder().with<ContractPayload>(contractE).build().each(
          [&](flecs::entity payloadETemp) { payloadE = payloadETemp; });

      if (payloadE.is_valid()) {
        world.query_builder().with<LaunchingWith>(payloadE).build().each(
            [&](flecs::entity launchPlanETemp) {
              launchPlanE = launchPlanETemp;
            });
      }

      ImGui::TableNextRow();
      ImGui::PushID(std::to_string(contractE.id()).c_str());

      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(contract.name.c_str());

      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(contract.client.c_str());

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(contract.description.c_str());

      ImGui::TableSetColumnIndex(3);
      auto contractState = contractE.target<ContractCurrentState>();
      const char *statusStr =
          contractState.is_valid() ? contractState.name().c_str() : "-";
      ImGui::TextUnformatted(statusStr);

      ImGui::TableSetColumnIndex(4);
      ImGui::Text("$%s", format_locale(contract.upfront_payment +
                                       contract.completion_payment)
                             .c_str());

      ImGui::TableSetColumnIndex(5);
      auto targetOrbit = contractE.target<ContractTargetOrbit>();
      ImGui::Text("%s",
                  targetOrbit.is_valid() ? targetOrbit.name().c_str() : "-");

      ImGui::TableSetColumnIndex(6);
      ImGui::TextUnformatted(contract.failed ? "Yes" : "No");

      ImGui::TableSetColumnIndex(7);

      ImGui::BeginDisabled(acceptButtonDisabled(contractE));
      if (ImGui::SmallButton("Accept")) {
        acceptContract(world, contractE);
        spdlog::debug("Contract {} accepted", contractE.id());
      }
      ImGui::EndDisabled();

      ImGui::SameLine();
      ImGui::BeginDisabled(rejectButtonDisabled(contractE));
      if (ImGui::SmallButton("Reject")) {
        rejectContract(world, contractE);
        spdlog::debug("Contract {} rejected", contractE.id());
      }
      ImGui::EndDisabled();

      ImGui::SameLine();
      ImGui::BeginDisabled(planButtonDisabled(contractE));
      if (ImGui::SmallButton(launchPlanE.is_valid() ? "View" : "Plan")) {
        if (launchPlanE.is_valid()) {
          showLaunchDetailWindow(launchPlanE);
        } else {
          setupLaunchForPayload(payloadE);
        }
      }
      ImGui::EndDisabled();

      ImGui::TableSetColumnIndex(8);
      if (contractE.has<ContractCurrentState>(
              world.lookup("States::Contract::Closed"))) {
        if (ImGui::SmallButton("Delete")) {
          state.pendingDelete = contractE;
          openDeleteModal = true;
        }
      }

      ImGui::PopID();
    });

    ImGui::EndTable();

    if (openDeleteModal) {
      ImGui::OpenPopup("Confirm Delete Contract");
    }
  }

  // --- Delete confirmation modal ---
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (ImGui::BeginPopupModal("Confirm Delete Contract", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (state.pendingDelete.is_valid() && state.pendingDelete.is_alive()) {
      ImGui::Text("Delete contract '%s'?",
                  state.pendingDelete.get<Contract>().name.c_str());
      ImGui::Text("This action cannot be undone.");
      ImGui::Separator();
      if (ImGui::Button("Yes, Delete")) {
        state.pendingDelete.destruct();
        state.pendingDelete = flecs::entity::null();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Keep")) {
        state.pendingDelete = flecs::entity::null();
        ImGui::CloseCurrentPopup();
      }
    } else {
      state.pendingDelete = flecs::entity::null();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}
