#include "contracts_window.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/engine/helpers.h"
#include "modules/rocket/contract_actions.h"
#include "modules/rocket/launch_window.h"
#include "modules/rocket/rocket_module.h"
#include "modules/window/contract_detail_window.h"
#include <flecs/addons/cpp/entity.hpp>
#include <flecs/addons/cpp/mixins/query/impl.hpp>
#include <string>

bool contractMatchesFilter(flecs::entity contractE,
                           const ContractListWindow &state) {
  const auto *contract = contractE.try_get<Contract>();
  if (!contract) {
    return false;
  }

  auto stateE = contractE.target<ContractCurrentState>();
  if (state.hideTerminal && stateE.is_valid() &&
      stateE.has<StateIsTerminal>()) {
    return false;
  }
  if (!state.stateFilter.is_valid()) {
    return true;
  }
  return contractE.has<ContractCurrentState>(state.stateFilter);
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

void openLaunchWindowForPayload(flecs::entity payloadE) {
  setupLaunchForPayload(payloadE);
}

void showContractListWindow(flecs::world &world) {
  showWindow(world, "Contract List");
}

void drawContractListWindow(flecs::entity winE) {
  auto &state = winE.get_mut<ContractListWindow>();
  auto world = winE.world();

  flecs::query<Contract> contractQuery =
      world.query_builder<Contract>().build();

  // --- Filter ---
  const char *filterPreview = "All";
  if (state.stateFilter.is_valid() && state.stateFilter.is_alive()) {
    filterPreview = state.stateFilter.has<Label>()
                        ? state.stateFilter.get<Label>().label.c_str()
                        : state.stateFilter.name().c_str();
  }
  ImGui::SetNextItemWidth(150.0f);
  if (ImGui::BeginCombo("##StateFilter", filterPreview)) {
    if (ImGui::Selectable("All", !state.stateFilter.is_valid())) {
      state.stateFilter = flecs::entity::null();
    }
    auto contractStates = world.lookup("States::Contract");
    if (contractStates.is_valid()) {
      contractStates.children([&](flecs::entity stateE) {
        const char *label = stateE.has<Label>()
                                ? stateE.get<Label>().label.c_str()
                                : stateE.name().c_str();
        if (ImGui::Selectable(label, state.stateFilter == stateE)) {
          state.stateFilter = stateE;
        }
      });
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  ImGui::Checkbox("Hide completed", &state.hideTerminal);
  ImGui::Separator();

  // --- Table ---
  constexpr ImGuiTableFlags tableFlags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
      ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY |
      ImGuiTableFlags_Resizable;
  if (ImGui::BeginTable("contracts", 6, tableFlags)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("Client");
    ImGui::TableSetupColumn("Total Payment", ImGuiTableColumnFlags_WidthFixed,
                            100.0f);
    ImGui::TableSetupColumn("Target Orbit", ImGuiTableColumnFlags_WidthFixed,
                            100.0f);
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f);
    ImGui::TableHeadersRow();

    contractQuery.each([&](flecs::entity contractE, Contract &contract) {
      if (!contractMatchesFilter(contractE, state)) {
        return;
      }

      ImGui::TableNextRow();
      ImGui::PushID(std::to_string(contractE.id()).c_str());

      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(contract.name.c_str());

      ImGui::TableSetColumnIndex(1);
      auto contractState = contractE.target<ContractCurrentState>();
      const char *statusStr =
          contractState.is_valid() && contractState.has<Label>()
              ? contractState.get<Label>().label.c_str()
              : (contractState.is_valid() ? contractState.name().c_str() : "-");
      ImGui::TextUnformatted(statusStr);

      ImGui::TableSetColumnIndex(2);
      ImGui::TextUnformatted(contract.client.c_str());

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("$%s", format_locale(contract.upfront_payment +
                                       contract.completion_payment)
                             .c_str());

      ImGui::TableSetColumnIndex(4);
      auto targetOrbit = contractE.target<ContractTargetOrbit>();
      ImGui::Text("%s",
                  targetOrbit.is_valid() ? targetOrbit.name().c_str() : "-");

      ImGui::TableSetColumnIndex(5);
      if (ImGui::SmallButton("View")) {
        showContractDetailWindow(contractE);
      }

      ImGui::PopID();
    });

    ImGui::EndTable();
  }
}
