#include "construction_window.h"
#include "imgui.h"
#include "modules/site/site.h"
#include "spdlog/fmt/bundled/core.h"
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

void buildPrefab(flecs::entity &constructionE, flecs::entity &prefabE) {
  auto world = constructionE.world();

  std::string name;
  int ii = 1;
  do {
    name = fmt::format("{} {}", prefabE.name().c_str(), ii++);
  } while (constructionE.parent().lookup(name.c_str()).is_valid());

  auto location = constructionE.get<SiteLocation>();

  world.entity(name.c_str())
      .is_a(prefabE)
      .set<SiteLocation>(location)
      .child_of(constructionE.parent());

  constructionE.parent().add<ConstructionSiteNeedsUpdating>();
}

/// @brief System encapsulating the draw commands for the ConstructionSiteWindow
/// @param[in] winE The entity for the window
/// @param[in] win The Component holding the window information
void systemDrawConstructionSiteWindow(flecs::entity winE,
                                      ConstructionSiteWindow &win) {
  auto world = winE.world();
  auto entity = win.buildingE;

  if (entity == flecs::entity() || !entity.is_alive() || !win.open) {
    spdlog::error("Building is no longer valid for ConstructionSiteWindow");
    hideConstructionSiteWindow(world);
    return;
  }

  auto buildingPrefabs = world.lookup("Prefabs::Buildings");
  if (!buildingPrefabs.is_valid()) {
    spdlog::error("Failed to load Building Prfabs!");
    return;
  }

  ImGui::SetNextWindowSize({170, 250}, ImGuiCond_FirstUseEver);
  ImGui::Begin(
      fmt::format("Construction Site ###ConstructionSiteWindow").c_str(),
      &win.open);

  auto buttonSize = ImGui::GetContentRegionAvail();
  buttonSize.y = 30;

  buildingPrefabs.children([&](flecs::entity prefabE) {
    ImGui::PushID(prefabE.id());

    if (ImGui::Button(fmt::format("{}", prefabE.name().c_str()).c_str(),
                      buttonSize)) {
      buildPrefab(entity, prefabE);
    }
    ImGui::PopID();
  });

  ImGui::End();
}
