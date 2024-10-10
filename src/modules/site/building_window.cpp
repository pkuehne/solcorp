#include "imgui.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "site.h"
#include "spdlog/spdlog.h"
#include "widgets/widgets.h"
#include <flecs.h>

void drawManufacturingSection(flecs::entity &entity);
void drawStorageSection(flecs::entity &entity);
void drawRocketButtons(flecs::entity &rocket);
void movePopup(flecs::entity &rocket);

void showBuildingWindow(const flecs::entity &entity) {
  spdlog::info("Showing SiteWindow");
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

  spdlog::info("Hiding BuildingWindow");
  auto entity = world.lookup("BuildingWindow");
  entity.destruct();
}

void systemDrawBuildingWindow(flecs::entity winE, BuildingWindow &win) {
  auto world = winE.world();
  auto entity = win.buildingE;
  if (entity == flecs::entity() || !entity.is_alive()) {
    spdlog::error("Building is no longer valid for BuildingWindow");
    hideBuildingWindow(world);
    return;
  }

  ImGui::Begin(
      fmt::format("Building - {}###BuildingWindow", entity.name().c_str())
          .c_str());
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
      ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
  }

  ImGui::End();
}

void drawManufacturingSection(flecs::entity &entity) {
  flecs::world world = entity.world();
  Manufacturing *manu = entity.get_mut<Manufacturing>();

  size_t index = 0;
  for (flecs::entity &e : manu->lines) {
    ImGui::PushID(index++);
    ImGui::SeparatorText(fmt::format("Line {}", index).c_str());

    if (e.is_valid()) {
      // There is a rocket on the line
      ImGui::Text("Constructing Rocket %ld", e.id());

      Construction *c = e.get_mut<Construction>();
      if (c) {
        float completed = c->effort_total - c->effort_remaining;
        ImGui::ProgressBar(completed / c->effort_total);
      } else {
        ImGui::Text(" ");
      }
      drawRocketButtons(e);
      // ImGui::SameLine();
      // if (ImGui::SmallButton("X")) {
      //   e.remove<Construction>();
      // }
    } else {
      // Nothing yet - the line is empty
      ImGui::Text("Empty Manufacturing Line");
      ImGui::Text(" ");
      if (ImGui::Button("Build")) {
        // Build new rocket
        e = world.entity()
                .is_a<Rocket>()
                .set<Construction>({300, 300})
                .child_of(entity);
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
    ImGui::Text("Rocket %ld", rocket.id());
    drawRocketButtons(rocket);
    ImGui::Separator();
  });
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
  if (rocket.has<LaunchingWith>(flecs::Wildcard)) {
    issue = "Rocket is already scheduled";
  }

  if (ActionButton("Schedule", "Schedule the rocket for launch", issue)) {
    // ImGui::OpenPopup("Schedule Launch");
    auto world = rocket.world();
    auto e = world.entity().set<LaunchPlan>({});
    showLaunchWindow(e);
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
  std::string issue =
      destination == flecs::entity() ? "Invalid destination" : "";
  if (ActionButton("Ok", nullptr, issue)) {
    rocket.child_of(destination);
    closePopup();
  }
  ImGui::EndPopup();
}
