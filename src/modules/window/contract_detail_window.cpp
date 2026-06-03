#include "contract_detail_window.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include "modules/engine/helpers.h"
#include "modules/rocket/contract_actions.h"
#include "modules/rocket/launch_detail_window.h"
#include "modules/rocket/rocket_module.h"
#include "modules/window/contracts_window.h"
#include "widgets/widgets.h"
#include <flecs.h>
#include <string>

namespace {

flecs::entity launchPlanForPayload(flecs::entity payloadE) {
  flecs::entity launchPlanE = flecs::entity::null();
  if (!payloadE.is_valid()) {
    return launchPlanE;
  }

  payloadE.world().query_builder().with<LaunchingWith>(payloadE).build().each(
      [&](flecs::entity e) {
        if (!launchPlanE.is_valid()) {
          launchPlanE = e;
        }
      });
  return launchPlanE;
}

} // namespace

void showContractDetailWindow(flecs::entity contractE) {
  auto world = contractE.world();
  auto win = showWindow(world, "Contract Detail");
  win.get_mut<ContractDetailWindow>().contract = contractE;
}

void drawContractDetailWindow(flecs::entity winE) {
  auto &state = winE.get_mut<ContractDetailWindow>();
  auto world = winE.world();

  if (!state.contract.is_valid() || !state.contract.is_alive() ||
      !state.contract.has<Contract>()) {
    hideWindow(world, "Contract Detail");
    return;
  }

  const auto &contract = state.contract.get<Contract>();
  auto payloadE = state.contract.target<ContractPayload>();
  auto launchPlanE = launchPlanForPayload(payloadE);
  auto targetOrbit = state.contract.target<ContractTargetOrbit>();
  auto statusE = state.contract.target<ContractCurrentState>();

  ImGui::Text("%s", contract.name.c_str());
  ImGui::SameLine();
  ImGui::TextDisabled("(%s)", statusE.name().c_str());
  ImGui::Separator();

  if (ImGui::BeginTable("##contractDetailLayout", 2,
                        ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("##info", ImGuiTableColumnFlags_WidthStretch,
                            0.58f);
    ImGui::TableSetupColumn("##mission", ImGuiTableColumnFlags_WidthStretch,
                            0.42f);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);

    ImGui::TextDisabled("Client");
    ImGui::TextUnformatted(contract.client.c_str());
    ImGui::Spacing();

    ImGui::TextDisabled("Description");
    ImGui::TextWrapped("%s", contract.description.c_str());
    ImGui::Spacing();

    ImGui::TextDisabled("Payments");
    if (ImGui::BeginTable(
            "##contractPayments", 2,
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("Type");
      ImGui::TableSetupColumn("Amount", ImGuiTableColumnFlags_WidthFixed,
                              120.0f);
      ImGui::TableHeadersRow();

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Upfront");
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("$%s", format_locale(contract.upfront_payment).c_str());

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Completion");
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("$%s", format_locale(contract.completion_payment).c_str());

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Total");
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("$%s", format_locale(contract.upfront_payment +
                                       contract.completion_payment)
                             .c_str());

      ImGui::EndTable();
    }
    ImGui::Spacing();

    ImGui::TextDisabled("Outcome");
    if (state.contract.has<ContractCurrentState>(
            world.lookup("States::Contract::Closed"))) {
      ImGui::TextUnformatted(contract.failed ? "Failed" : "Completed");
    } else {
      ImGui::TextUnformatted("Pending");
    }

    ImGui::TableSetColumnIndex(1);

    ImGui::TextDisabled("Target Orbit");
    ImGui::TextUnformatted(targetOrbit.is_valid() ? targetOrbit.name().c_str()
                                                  : "-");
    ImGui::Spacing();

    ImGui::TextDisabled("Payload");
    if (payloadE.is_valid() && payloadE.has<Payload>()) {
      const auto &payload = payloadE.get<Payload>();
      ImGui::Text("%s (%s kg)", payloadE.name().c_str(),
                  format_locale(payload.mass).c_str());
    } else if (payloadE.is_valid()) {
      ImGui::TextUnformatted(payloadE.name().c_str());
    } else {
      ImGui::TextUnformatted("-");
    }
    ImGui::Spacing();

    ImGui::TextDisabled("Launch Plan");
    if (launchPlanE.is_valid()) {
      ImGui::Text("%s", launchPlanE.name().c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("View##launchPlan")) {
        showLaunchDetailWindow(launchPlanE);
      }
    } else {
      ImGui::TextUnformatted("None");
      ImGui::SameLine();
      auto planIssue =
          planButtonDisabled(state.contract)
              ? "Contract is not accepted"
              : (!payloadE.is_valid() ? "Contract has no payload" : "");
      if (Widgets::SmallActionButton(
              ButtonLabel{.text = "Plan"},
              ButtonTooltip{.text = "Create a launch plan for this contract"},
              planIssue)) {
        openLaunchWindowForPayload(payloadE);
      }
    }

    ImGui::EndTable();
  }

  ImGui::Spacing();
  ImGui::Separator();

  auto acceptValid = ContractAcceptAction{state.contract}.validate(world);
  if (Widgets::ActionButton(ButtonLabel{.text = "Accept"},
                            ButtonTooltip{.text = "Accept this contract"},
                            acceptValid.message)) {
    acceptContract(world, state.contract);
  }

  ImGui::SameLine();
  auto rejectValid = ContractRejectAction{state.contract}.validate(world);
  if (Widgets::ActionButton(ButtonLabel{.text = "Reject"},
                            ButtonTooltip{.text = "Reject this contract"},
                            rejectValid.message)) {
    rejectContract(world, state.contract);
  }

  ImGui::SameLine();
  if (ImGui::Button("Close")) {
    hideWindow(world, "Contract Detail");
    state.contract = flecs::entity::null();
  }
}
