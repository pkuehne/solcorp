#include "building_detail_window.h"
#include "imgui.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/rocket/launch_detail_window.h"
#include "modules/rocket/launch_window.h"
#include "modules/rocket/rocket_module.h"
#include "modules/site/rocket_prefab_window.h"
#include "modules/site/site.h"
#include "rocket_detail_window.h"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include "storage_picker.h"
#include "widgets/stat_widget.h"
#include "widgets/widgets.h"
#include <flecs.h>
#include <flecs/addons/cpp/entity.hpp>
#include <functional>
#include <modules/simulation/simulation.h>

void drawManufacturingSection(flecs::entity &entity);
void drawStorageSection(flecs::entity &entity);
void drawLaunchpadSection(flecs::entity &entity);

void showBuildingDetailWindow(const flecs::entity &entity) {
  spdlog::debug("Showing BuildingDetailWindow");
  if (!entity.is_alive()) {
    spdlog::error(
        "showing BuildingDetailWindow can't be done on invalid building");
    return;
  }

  auto world = entity.world();
  auto window = showWindow(world, "Building Detail");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Building Detail");
  auto win = window.try_get_mut<Window>();
  SC_ASSERT(win, "Window state is invalid");
  win->title = fmt::format("Building Detail ({})", entity.name().c_str());
  auto state = window.try_get_mut<BuildingDetailWindow>();
  SC_ASSERT(state, "BuildingDetailWindow state is invalid");
  state->buildingE = entity;
}

void drawBuildingDetailWindow(flecs::entity winE) {
  auto &state = winE.get_mut<BuildingDetailWindow>();
  auto world = winE.world();

  auto buildingE = state.buildingE;
  if (buildingE == flecs::entity() || !buildingE.is_alive()) {
    spdlog::error("Building is no longer valid for BuildingDetailWindow");
    hideWindow(world, "Building Detail");
    return;
  }

  if (ImGui::BeginTabBar("Facilities")) {
    auto query = world.query_builder()
                     .with<Facility>()
                     .with(flecs::ChildOf, buildingE)
                     .build();

    query.each([](flecs::entity facilityE) {
      if (ImGui::BeginTabItem(facilityE.name().c_str())) {
        if (facilityE.has<Manufacturing>()) {
          ImGui::SeparatorText("Manufacturing");
          drawManufacturingSection(facilityE);
        }
        if (facilityE.has<Storage>()) {
          ImGui::SeparatorText("Storage");
          drawStorageSection(facilityE);
        }
        if (facilityE.has<Office>()) {
          ImGui::SeparatorText("Offices");
          // Currently no office section
        }
        if (facilityE.has<Launchpad>()) {
          ImGui::SeparatorText("Launchpad");
          drawLaunchpadSection(facilityE);
        }
        ImGui::EndTabItem();
      }
    });
    ImGui::EndTabBar();
  }
}

void drawManufacturingSection(flecs::entity &entity) {
  flecs::world world = entity.world();

  flecs::entity e = flecs::entity::null();
  entity.children([&](flecs::entity ch) {
    if (ch.has<Rocket>()) {
      e = ch;
    }
  });

  if (e.is_valid()) {
    // There is a rocket on the line
    ImGui::Text("Constructing %s", e.name().c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("View")) {
      showRocketDetailWindow(e);
    }
    ImGui::ProgressBar(getEntityEffortRequired(e));

  } else {
    // Nothing yet - the line is empty
    ImGui::Text("Empty Manufacturing Line");
    ImGui::ProgressBar(0.0);

    if (Widgets::ActionButton(
            ButtonLabel{.text = "Build"},
            ButtonTooltip{.text = "Select a rocket prefab to build"}, "")) {
      showRocketPrefabWindow(entity);
    }
  }
  flecs::entity targetPrefab = entity.target<ManufacturingLineTemplate>();
  ImGui::Text("Tooled for: %s",
              targetPrefab.is_valid() ? targetPrefab.name().c_str() : "None");
  flecs::entity targetStorage = entity.target<ManufacturingLineStorage>();
  ImGui::Text("Stored to: %s ", targetStorage.is_valid()
                                    ? targetStorage.parent().name().c_str()
                                    : "Here");
  ImGui::SameLine();
  bool openSetStorage = false;
  if (ImGui::SmallButton(targetStorage.is_valid() ? "Clear" : "Set")) {
    if (targetStorage.is_valid()) {
      entity.remove<ManufacturingLineStorage>();
    } else {
      openSetStorage = true;
    }
  }
  storagePickerPopup(
      "Set Storage", openSetStorage, world, flecs::entity(),
      [&](flecs::entity dest) { entity.add<ManufacturingLineStorage>(dest); });
  ImGui::Separator();

  // Settings
  auto &manufacturing = entity.get_mut<Manufacturing>();
  ImGui::Checkbox("Auto Build", &manufacturing.auto_build_next);
  if (ImGui::BeginItemTooltip()) {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
    ImGui::TextUnformatted(
        "If enabled, once the current manufacturing effort is complete, a "
        "new rocket of the same model will automatically be added to the line "
        "to be constructed. ");
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
  ImGui::Checkbox("Auto Store", &manufacturing.auto_store);
  if (ImGui::BeginItemTooltip()) {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
    ImGui::TextUnformatted(
        "If enabled, once the current manufacturing effort is complete, "
        "the completed rocket will be automatically moved to the selected "
        "storage ");
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void drawStorageSection(flecs::entity &entity) {
  entity.children([](flecs::entity rocket) {
    if (!rocket.has<Rocket>()) {
      return;
    }
    ImGui::PushID(std::to_string(rocket.id()).c_str());
    ImGui::TextUnformatted(rocket.name().c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("View##rocket")) {
      showRocketDetailWindow(rocket);
    }
    auto plan = rocket.target<LaunchingOn>();
    if (plan.is_valid()) {
      ImGui::SameLine();
      ImGui::Text("(%s)", plan.name().c_str());
      ImGui::SameLine();
      if (ImGui::SmallButton("View##plan")) {
        showLaunchDetailWindow(plan);
      }
    }
    ImGui::Separator();
    ImGui::PopID();
  });
}

/// @brief Draws the "Launchpad" tab on a building
/// @param[in] entity The Building entity
void drawLaunchpadSection(flecs::entity &entity) {
  auto world = entity.world();

  const auto &launchpad = entity.get<Launchpad>();
  Widgets::StatTooltip(world, &launchpad.max_weight);
  Widgets::StatTooltip(world, &launchpad.prep_days);

  ImGui::Separator();
  activeLaunchPlansForPad(entity).each([](flecs::entity planE,
                                          LaunchPlan &plan) {
    ImGui::PushID(std::to_string(planE.id()).c_str());
    ImGui::Text("%s launching on %d", planE.name().c_str(), plan.launch_date);
    ImGui::SameLine();
    if (ImGui::SmallButton("View")) {
      showLaunchDetailWindow(planE);
    }
    ImGui::PopID();
  });
  ImGui::Separator();
  if (ImGui::Button("Schedule Launch")) {
    LaunchWindowDraftOptions options;
    options.launchpad = entity;
    showLaunchWindowAdd(world, options);
  }
}

flecs::query<LaunchPlan> activeLaunchPlansForPad(flecs::entity padE) {
  // Match plans on this pad whose current-state entity ($state) is not
  // terminal. Filtering at the query level avoids iterating over completed
  // launches, which accumulate without bound as the game progresses.
  return padE.world()
      .query_builder<LaunchPlan>()
      .with<LaunchingFrom>(padE)
      .with<LaunchPlanCurrentState>()
      .second("$state")
      .with<StateIsTerminal>()
      .src("$state")
      .not_()
      .build();
}

float getEntityEffortRequired(flecs::entity &entity) {

  auto *c = entity.try_get_mut<EffortRequired>();
  if (!c) {
    return 1.0f;
  }

  auto total = static_cast<float>(c->total);
  auto remaining = static_cast<float>(c->remaining);

  return 1.0f - (remaining / total);
}
