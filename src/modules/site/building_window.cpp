#include "imgui.h"
#include "site.h"
#include "spdlog/spdlog.h"
#include <flecs.h>

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
  ImGui::SeparatorText("General");
  ImGui::Text("Name: %s", entity.name().c_str());

  ImGui::End();
}
