#include "launch_detail_window.h"
#include "imgui.h"
#include "launch_actions.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/window/building_detail_window.h"
#include "modules/window/rocket_detail_window.h"
#include "rocket_module.h"
#include "widgets/widgets.h"
#include <flecs.h>

void showLaunchDetailWindow(flecs::entity planE) {
  auto world = planE.world();
  auto win = showWindow(world, "Launch Detail");
  win.get_mut<LaunchDetailWindow>().plan = planE;
}

void drawLaunchDetailWindow(flecs::entity winE) {
  auto &state = winE.get_mut<LaunchDetailWindow>();
  auto world = winE.world();

  if (!state.plan.is_valid() || !state.plan.is_alive()) {
    hideWindow(world, "Launch Detail");
    return;
  }

  auto &planData = state.plan.get<LaunchPlan>();
  auto rocketE = state.plan.target<LaunchingOn>();
  auto launchpadE = state.plan.target<LaunchingFrom>();

  bool isScheduled = state.plan.has<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::Scheduled"));
  bool isRollingOut = state.plan.has<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::RollingOut"));
  bool isOnPad = state.plan.has<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::OnPad"));
  bool isLaunched = state.plan.has<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::Launched"));
  bool isCancelled = state.plan.has<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::Cancelled"));

  ImGui::Text("%s", state.plan.name().c_str());
  ImGui::SameLine();
  const char *stateName = isScheduled    ? "Scheduled"
                          : isRollingOut ? "Rolling Out"
                          : isOnPad      ? "On Pad"
                          : isLaunched   ? "Launched"
                          : isCancelled  ? "Cancelled"
                                         : "Unknown";
  ImGui::TextDisabled("(%s)", stateName);
  ImGui::Separator();

  uint32_t today = world.get<Game>().day;

  if (ImGui::BeginTable("##detailLayout", 2,
                        ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("##stages", ImGuiTableColumnFlags_WidthStretch,
                            0.6f);
    ImGui::TableSetupColumn("##info", ImGuiTableColumnFlags_WidthStretch, 0.4f);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    if (ImGui::BeginTable(
            "##stagesTable", 3,
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("Day", ImGuiTableColumnFlags_WidthFixed, 60.0f);
      ImGui::TableSetupColumn("Stage");
      ImGui::TableSetupColumn("Status");
      ImGui::TableHeadersRow();

      auto countdown = [&](uint32_t day) {
        if (today >= day) {
          ImGui::TextDisabled("today");
        } else {
          uint32_t n = day - today;
          ImGui::TextDisabled("in %u day%s", n, n == 1 ? "" : "s");
        }
      };

      // Rollout row
      ImGui::TableNextRow();
      bool rolloutDone = isOnPad || isLaunched;
      bool rolloutActive = isRollingOut;
      if (rolloutActive) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                               IM_COL32(255, 220, 60, 40));
      }
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%u", planData.rollout_date);
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted("Rollout");
      ImGui::TableSetColumnIndex(2);
      if (rolloutDone) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Done");
      } else if (rolloutActive) {
        if (rocketE.is_valid() && rocketE.has<DurationRequired>()) {
          auto dur = rocketE.get<DurationRequired>();
          ImGui::Text("%u/%u days", dur.remaining, dur.total);
        } else {
          ImGui::TextUnformatted("In progress");
        }
      } else {
        countdown(planData.rollout_date);
      }

      // On Pad row
      ImGui::TableNextRow();
      bool onPadActive = isOnPad;
      if (onPadActive) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                               IM_COL32(255, 220, 60, 40));
      }
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%u", planData.prep_date);
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted("On Pad");
      ImGui::TableSetColumnIndex(2);
      if (isLaunched) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Done");
      } else if (onPadActive) {
        if (state.plan.has<DurationRequired>()) {
          auto dur = state.plan.get<DurationRequired>();
          ImGui::Text("%u/%u days", dur.remaining, dur.total);
        } else {
          ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Ready");
        }
      } else {
        countdown(planData.prep_date);
      }

      // Launch row
      ImGui::TableNextRow();
      bool launchReady = isOnPad && !state.plan.has<DurationRequired>() &&
                         today >= planData.launch_date;
      if (isLaunched || launchReady) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                               IM_COL32(60, 220, 60, 40));
      }
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%u", planData.launch_date);
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted("Launch");
      ImGui::TableSetColumnIndex(2);
      if (isLaunched) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Done");
      } else if (launchReady) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Ready");
      } else {
        countdown(planData.launch_date);
      }

      ImGui::EndTable();
    }

    ImGui::TableSetColumnIndex(1);

    ImGui::TextDisabled("Rocket");
    if (rocketE.is_valid()) {
      ImGui::TextUnformatted(rocketE.name().c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("View##rocket")) {
        showRocketDetailWindow(rocketE);
      }
    } else {
      ImGui::TextDisabled("None");
    }
    ImGui::Spacing();

    ImGui::TextDisabled("Launchpad");
    if (launchpadE.is_valid()) {
      ImGui::TextUnformatted(launchpadE.name().c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("View##pad")) {
        showBuildingDetailWindow(launchpadE.parent());
      }
    } else {
      ImGui::TextDisabled("None");
    }
    ImGui::Spacing();

    ImGui::TextDisabled("Orbit");
    ImGui::TextUnformatted(planData.target_orbit.is_valid()
                               ? planData.target_orbit.name().c_str()
                               : "-");
    ImGui::Spacing();

    ImGui::TextDisabled("Payloads");
    bool anyPayloads = false;
    state.plan.each<LaunchingWith>([&](flecs::entity payload) {
      if (payload.is_valid() && payload.has<Payload>()) {
        ImGui::Text("  %s (%u kg)", payload.name().c_str(),
                    payload.get<Payload>().mass);
        anyPayloads = true;
      }
    });
    if (!anyPayloads) {
      ImGui::TextDisabled("  None");
    }

    ImGui::EndTable();
  }

  ImGui::Spacing();
  ImGui::Separator();

  LaunchCancelAction cancelAction{state.plan};
  auto cancelValid = cancelAction.validate(world);
  if (Widgets::ActionButton(ButtonLabel{.text = "Cancel Plan"},
                            ButtonTooltip{.text = "Cancel this launch plan"},
                            cancelValid.message)) {
    ImGui::OpenPopup(modalCancelLaunch);
  }
  ImGui::SameLine();
  if (ImGui::Button("Close")) {
    hideWindow(world, "Launch Detail");
    state.plan = flecs::entity::null();
  }

  if (drawLaunchCancelConfirmModal(state.plan, world)) {
    state.plan = flecs::entity::null();
    hideWindow(world, "Launch Detail");
  }
}

bool drawLaunchCancelConfirmModal(flecs::entity plan, flecs::world &world) {
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  if (!ImGui::BeginPopupModal(modalCancelLaunch, nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize)) {
    return false;
  }
  bool executed = false;
  if (plan.is_valid() && plan.is_alive()) {
    ImGui::Text("Cancel launch plan '%s'?", plan.name().c_str());
    ImGui::Text("This action cannot be undone.");
    ImGui::Separator();
    auto confirmValid = LaunchCancelAction{plan}.validate(world);
    if (Widgets::ActionButton(ButtonLabel{.text = "Yes, Cancel Launch"},
                              ButtonTooltip{.text = nullptr},
                              confirmValid.message)) {
      LaunchCancelAction{plan}.execute(world);
      ImGui::CloseCurrentPopup();
      executed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Keep")) {
      ImGui::CloseCurrentPopup();
    }
  } else {
    ImGui::CloseCurrentPopup();
  }
  ImGui::EndPopup();
  return executed;
}
