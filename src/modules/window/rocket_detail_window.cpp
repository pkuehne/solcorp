#include "rocket_detail_window.h"
#include "building_detail_window.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/engine/helpers.h"
#include "modules/rocket/launch_detail_window.h"
#include "modules/rocket/launch_window.h"
#include "modules/rocket/rocket_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/site/site.h"
#include "storage_picker.h"
#include "widgets/stat_widget.h"
#include "widgets/widgets.h"
#include <flecs.h>
#include <functional>

void showRocketDetailWindow(flecs::entity rocketE) {
  auto world = rocketE.world();
  auto win = showWindow(world, "Rocket Detail");
  win.get_mut<RocketDetailWindow>().rocket = rocketE;
}

void drawRocketDetailWindow(flecs::entity winE) {
  auto &state = winE.get_mut<RocketDetailWindow>();
  auto world = winE.world();

  if (!state.rocket.is_valid() || !state.rocket.is_alive()) {
    hideWindow(world, "Rocket Detail");
    return;
  }

  auto &rocketData = state.rocket.get<Rocket>();
  auto stateE = state.rocket.target<RocketCurrentState>();
  const char *stateName =
      stateE.is_valid() && stateE.has<Label>()
          ? stateE.get<Label>().label.c_str()
          : (stateE.is_valid() ? stateE.name().c_str() : "Unknown");

  std::string actionIssue;
  if (!state.rocket.has<RocketCurrentState>(
          world.lookup("States::Rocket::Stored"))) {
    actionIssue = "Rocket is not available";
  }

  bool openMovePopup = false;

  ImGui::Text("%s", state.rocket.name().c_str());
  ImGui::SameLine();
  ImGui::TextDisabled("(%s)", stateName);
  ImGui::Separator();

  if (ImGui::BeginTable("##rocketDetailLayout", 2,
                        ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("##info", ImGuiTableColumnFlags_WidthStretch,
                            0.55f);
    ImGui::TableSetupColumn("##stats", ImGuiTableColumnFlags_WidthStretch,
                            0.45f);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);

    auto prefabE = state.rocket.target(flecs::IsA);
    ImGui::TextDisabled("Model");
    ImGui::TextUnformatted(prefabE.is_valid() ? prefabE.name().c_str() : "-");
    ImGui::Spacing();

    auto facilityE = state.rocket.parent();
    auto buildingE =
        facilityE.is_valid() ? facilityE.parent() : flecs::entity();
    ImGui::TextDisabled("Location");
    if (buildingE.is_valid() && buildingE.has<Building>()) {
      ImGui::Text("%s", buildingE.name().c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("View##building")) {
        showBuildingDetailWindow(buildingE);
      }
      ImGui::SameLine();
      openMovePopup = Widgets::SmallActionButton(
          ButtonLabel{.text = "Move"},
          ButtonTooltip{.text =
                            "Move the rocket to another storage at this site"},
          actionIssue);
    } else if (facilityE.is_valid()) {
      ImGui::TextUnformatted(facilityE.name().c_str());
    } else {
      ImGui::TextUnformatted("-");
    }
    ImGui::Spacing();

    auto planE = state.rocket.target<LaunchingOn>();
    ImGui::TextDisabled("Plan");
    if (planE.is_valid()) {
      ImGui::Text("%s", planE.name().c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("View##plan")) {
        showLaunchDetailWindow(planE);
      }
    } else {
      ImGui::TextUnformatted("None");
      ImGui::SameLine();
      if (Widgets::SmallActionButton(
              ButtonLabel{.text = "Schedule"},
              ButtonTooltip{.text = "Schedule the rocket for launch"},
              actionIssue)) {
        showLaunchWindowAdd(world, &state.rocket, nullptr);
      }
    }
    ImGui::Spacing();

    if (ImGui::BeginTable(
            "##liftCapacity", 2,
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("Orbit");
      ImGui::TableSetupColumn("Max Payload", ImGuiTableColumnFlags_WidthFixed,
                              90.0f);
      ImGui::TableHeadersRow();

      bool anyOrbits = false;
      state.rocket.each<CanLiftTo>([&](flecs::entity orbit) {
        if (!orbit.is_valid()) {
          return;
        }
        auto &liftTo = state.rocket.get<CanLiftTo>(orbit);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(orbit.name().c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::Text(
            "%s kg",
            format_locale(static_cast<int64_t>(liftTo.max_mass)).c_str());
        anyOrbits = true;
      });

      if (!anyOrbits) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("None");
      }

      ImGui::EndTable();
    }

    ImGui::TableSetColumnIndex(1);

    ImGui::TextDisabled("Stats");
    Widgets::StatTooltip(&rocketData.failure_rate);
    Widgets::StatTooltip(&rocketData.cost);
    Widgets::StatTooltip(&rocketData.rollout_days);
    Widgets::StatTooltip(&rocketData.move_days);

    ImGui::EndTable();
  }

  auto currentStorage = state.rocket.parent();
  auto site = findAncestorWith<Site>(currentStorage);
  if (site.is_valid()) {
    auto moveDurationDays = static_cast<uint8_t>(rocketData.move_days.value());
    storagePickerPopup("Move Rocket", openMovePopup, world, currentStorage,
                       [&](flecs::entity dest) {
                         RocketMoveAction action{RocketEntity{state.rocket},
                                                 DestinationEntity{dest},
                                                 moveDurationDays};
                         action.execute(world);
                       });
  }

  ImGui::Spacing();
  ImGui::Separator();

  if (ImGui::Button("Close")) {
    hideWindow(world, "Rocket Detail");
    state.rocket = flecs::entity::null();
  }
}
