#include "building_window.h"
#include "imgui.h"
#include "modules/rocket_launch/actions.h"
#include "modules/rocket_launch/launch_window.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/stats/stats.h"
#include "site.h"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include "widgets/widgets.h"
#include <flecs.h>

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

  auto win = BuildingWindow();
  win.buildingE = entity;
  world.entity("BuildingWindow").set<BuildingWindow>(win);
}

void hideBuildingWindow(flecs::world &world) {
  spdlog::debug("Hiding BuildingWindow");
  auto entity = world.lookup("BuildingWindow");
  entity.destruct();
}

void systemDrawBuildingWindow(flecs::entity winE, BuildingWindow &win) {
  auto world = winE.world();
  auto entity = win.buildingE;
  if (entity == flecs::entity() || !entity.is_alive() || !win.open) {
    spdlog::error("Building is no longer valid for BuildingWindow");
    hideBuildingWindow(world);
    return;
  }

  ImGui::Begin(
      fmt::format("Building - {}###BuildingWindow", entity.name().c_str())
          .c_str(),
      &win.open);
  if (ImGui::BeginTabBar("Capabilities")) {
    if (entity.has<Manufacturing>() && ImGui::BeginTabItem("Manufacturing")) {
      drawManufacturingSection(entity);
      ImGui::EndTabItem();
    }
    if (entity.has<Storage>() && ImGui::BeginTabItem("Storage")) {
      drawStorageSection(entity);
      ImGui::EndTabItem();
    }
    if (entity.has<Office>() && ImGui::BeginTabItem("Office")) {
      ImGui::EndTabItem();
    }
    if (entity.has<Launchpad>() && ImGui::BeginTabItem("Launchpad")) {
      drawLaunchpadSection(entity);
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::End();
}

void drawManufacturingSection(flecs::entity &entity) {
  flecs::world world = entity.world();
  // Manufacturing &manu = entity.get_mut<Manufacturing>();

  size_t index = 0;
  {
    flecs::entity e = flecs::entity::null();
    entity.children([&](flecs::entity ch) {
      if (ch.has<Rocket>()) {
        e = ch;
      }
    });
    ImGui::PushID(index++);
    ImGui::SeparatorText(fmt::format("Line {}", index).c_str());

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
      ImGui::Text("Empty Manufacturing Line");
      ImGui::ProgressBar(0.0);
      if (ImGui::Button("Build")) {
        // Build new rocket
        // TODO: Move to RocketLaunch Module
        auto prefab = world.lookup("Prefabs::Core::Rocket");
        assert(prefab.is_valid());
        e = world.entity()
                .is_a(prefab)
                .set<Construction>({300, 300})
                .child_of(entity);
        e.set_name(fmt::format("Rocket {}", Rocket::max_id++).c_str());
      }
    }
    ImGui::PopID();
  }
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
  auto source = rocket.parent();

  ImGui::Text("Where to?");
  ImGui::SameLine();
  static flecs::entity destination;
  static std::string display = "<Select one>";

  flecs::query<> storageBuildings = world.query_builder()
                                        .with<Storage>()
                                        .with(flecs::ChildOf, source.parent())
                                        .build();

  if (ImGui::BeginCombo("##StorageCombo", display.c_str())) {
    storageBuildings.each([&](flecs::entity s) {
      if (s == source) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Selectable(s.name().c_str(), destination == s)) {
        destination = s;
        display = s.name();
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
