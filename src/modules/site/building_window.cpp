#include "building_window.h"
#include "imgui.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/rocket_launch/actions.h"
#include "modules/rocket_launch/launch_window.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/stats/stats.h"
#include "rocket_prefab_window.h"
#include "site.h"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include "widgets/widgets.h"
#include <flecs.h>
#include <modules/simulation/simulation.h>

void drawManufacturingSection(flecs::entity &entity);
void drawStorageSection(flecs::entity &entity);
void drawLaunchpadSection(flecs::entity &entity);
void drawRocketButtons(flecs::entity &rocket);
void movePopup(flecs::entity &rocket);

void showBuildingWindow(const flecs::entity &entity) {
  spdlog::debug("Showing BuildingWindow");
  if (!entity.is_alive()) {
    spdlog::error("showing BuildingWindow can't be done on invalid building");
    return;
  }

  auto world = entity.world();
  auto window = showWindow(world, "Building Window");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Building Window");
  auto win = window.try_get_mut<Window>();
  SC_ASSERT(win, "Window state is invalid");
  win->title = fmt::format("Building Window ({})", entity.name().c_str());
  auto state = window.try_get_mut<BuildingWindow>();
  SC_ASSERT(state, "BuildingWindow state is invalid");
  state->buildingE = entity;
}

void drawBuildingWindow(flecs::entity winE) {
  auto &state = winE.get_mut<BuildingWindow>();
  auto world = winE.world();

  auto buildingE = state.buildingE;
  if (buildingE == flecs::entity() || !buildingE.is_alive()) {
    spdlog::error("Building is no longer valid for BuildingWindow");
    hideWindow(world, "Building Window");
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
  ImGui::PushID(entity.id());

  if (e.is_valid()) {
    // There is a rocket on the line
    ImGui::Text("Constructing %s", e.name().c_str());

    Construction *c = e.try_get_mut<Construction>();
    if (c) {
      float completed = c->effort_total - c->effort_remaining;
      ImGui::ProgressBar(completed / c->effort_total);
    } else {
      ImGui::ProgressBar(1.0);
    }
    drawRocketButtons(e);
    // ImGui::SameLine();
    // if (ImGui::SmallButton("X")) {
    //   e.remove<Construction>();
    // }
  } else {
    // Nothing yet - the line is empty
    const int rocket_cost = 5'000'000;
    Company &company = world.get_mut<Company>();
    ImGui::Text("Empty Manufacturing Line");
    ImGui::ProgressBar(0.0);

    if (ActionButton("Build", "Start building a new rocket on this line",
                     company.balance < rocket_cost ? "Not enough funds" : "")) {
      showRocketPrefabWindow(entity);
    }
  }
  ImGui::PopID();
}

void drawStorageSection(flecs::entity &entity) {
  flecs::world world = entity.world();
  entity.children([](flecs::entity rocket) {
    if (rocket.has<Construction>()) {
      return;
    }
    ImGui::PushID(rocket.id());
    auto plan = rocket.target<LaunchingOn>();
    ImGui::Text("%s %s", rocket.name().c_str(),
                plan.is_valid()
                    ? fmt::format("({})", plan.name().c_str()).c_str()
                    : "");
    drawRocketButtons(rocket);
    ImGui::Separator();
    ImGui::PopID();
  });
}

/// @brief Draws the "Launchpad" tab on a building
/// @param[in] entity The Building entity
void drawLaunchpadSection(flecs::entity &entity) {
  auto world = entity.world();

  auto launchpad = entity.get<Launchpad>();
  displayStatWithTooltip(&launchpad.max_weight);
  displayStatWithTooltip(&launchpad.prep_days);

  ImGui::Separator();
  flecs::query<LaunchPlan> query =
      world.query_builder<LaunchPlan>().with<LaunchingFrom>(entity).build();
  query.each([](flecs::entity planE, LaunchPlan &plan) {
    ImGui::PushID(planE.id());
    ImGui::Text("%s launching on %d", planE.name().c_str(), plan.launch_date);
    ImGui::SameLine();
    if (ImGui::SmallButton("Open")) {
      ImGui::OpenPopup("Not Implemented");
      showLaunchWindowEdit(planE);
    }
    ImGui::PopID();
  });
  ImGui::Separator();
  if (ImGui::Button("Schedule Launch")) {
    showLaunchWindowAdd(world, nullptr, &entity);
  }
}

void drawRocketButtons(flecs::entity &rocket) {
  std::string issue;
  if (rocket.has<Construction>()) {
    issue = "Cannot move rocket while being built";
  }
  if (ActionButton("Move", "Move the rocket to another storage at this site",
                   issue)) {
    ImGui::OpenPopup("Move Rocket");
  }
  ImGui::SameLine();
  if (rocket.has<Construction>()) {
    issue = "Cannot schedule rocket while being built";
  }

  auto target = rocket.target<LaunchingOn>();
  std::string tooltip = "Schedule the rocket for launch";
  if (target.is_valid()) {
    tooltip = "Edit launch plan";
  }
  if (ActionButton("Schedule", tooltip.c_str(), issue)) {
    if (target.is_valid()) {
      showLaunchWindowEdit(target);
    } else {
      showLaunchWindowAdd(rocket.world(), &rocket, nullptr);
    }
  }
  movePopup(rocket);
}

void movePopup(flecs::entity &rocket) {
  if (!ImGui::BeginPopupModal("Move Rocket")) {
    return;
  }
  auto world = rocket.world();
  auto source = findAncestorWith<Site>(rocket.parent());
  if (!source.is_valid()) {
    spdlog::error("Error: Could not find Site for this rocket");
    return;
  }
  ImGui::Text("Where to?");
  ImGui::SameLine();
  static flecs::entity destination;
  static std::string display = "<Select one>";

  auto storageFacilities =
      world.query_builder().with<Storage>().with<Site>().up().build();

  if (ImGui::BeginCombo("##StorageCombo", display.c_str())) {
    storageFacilities.each([&](flecs::entity s) {
      if (s == source) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Selectable(s.parent().name().c_str(), destination == s)) {
        destination = s;
        display = s.parent().name();
      }
      if (s == source) {
        ImGui::EndDisabled();
      }
    });
    ImGui::EndCombo();
  }
  ImGui::Separator();
  auto closePopup = [&]() {
    // Reset variables when closing popup
    destination = flecs::entity();
    display = "<Select one>";
    ImGui::CloseCurrentPopup();
  };
  if (ImGui::Button("Cancel")) {
    closePopup();
  }
  ImGui::SameLine();
  MoveRocketAction action{rocket, destination};
  if (ActionButton("Ok", nullptr, action.validate(world).message)) {
    action.execute(world);
    closePopup();
  }
  ImGui::EndPopup();
}
