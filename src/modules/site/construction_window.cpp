#include "construction_window.h"
#include "imgui.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

/// @brief Wrapper to show the ConstructionSiteWindow
/// @param[in] entity The construction site to show the window for
void showConstructionSiteWindow(const flecs::entity &entity) {
  spdlog::debug("Showing ConstructionSiteWindow");
  if (!entity.is_alive()) {
    spdlog::error(
        "showing ConstructionSiteWindow can't be done on invalid building");
    return;
  }

  auto world = entity.world();

  auto win = ConstructionSiteWindow();
  win.buildingE = entity;
  world.entity("ConstructionSiteWindow").set<ConstructionSiteWindow>(win);
}

/// @brief Wrapper to hide the ConstructionSiteWindow
/// @param[in] world The world
void hideConstructionSiteWindow(flecs::world &world) {

  spdlog::debug("Hiding ConstructionSiteWindow");
  auto entity = world.lookup("ConstructionSiteWindow");
  entity.destruct();
}

/// @brief System encapsulating the draw commands for the ConstructionSiteWindow
/// @param[in] winE The entity for the window
/// @param[in] win The Component holding the window information
void systemDrawConstructionSiteWindow(flecs::entity winE,
                                      ConstructionSiteWindow &win) {
  auto world = winE.world();
  auto entity = win.buildingE;

  if (entity == flecs::entity() || !entity.is_alive()) {
    spdlog::error("Building is no longer valid for ConstructionSiteWindow");
    hideConstructionSiteWindow(world);
    return;
  }

  ImGui::Begin(
      fmt::format("Construction Site ###ConstructionSiteWindow").c_str());

  ImGui::End();
}
