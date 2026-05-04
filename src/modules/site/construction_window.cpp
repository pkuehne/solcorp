#include "construction_window.h"
#include "imgui.h"
#include "modules/base/assert.h"
#include "modules/engine/gui.h"
#include "modules/engine/render.h"
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

  auto window = showWindow(world, "Construction Site Window");
  SC_ASSERT(window.is_valid(),
            "showWindow returned invalid entity for Construction Site Window");
  auto state = window.try_get_mut<ConstructionSiteWindow>();
  SC_ASSERT(state, "ConstructionSiteWindow state is invalid");
  state->buildingE = entity;
}

void buildPrefab(flecs::entity &constructionE, flecs::entity &prefabE) {
  auto world = constructionE.world();

  std::string name;
  int ii = 1;
  do {
    name = fmt::format("{} {}", prefabE.name().c_str(), ii++);
  } while (constructionE.parent().lookup(name.c_str()).is_valid());

  auto location = constructionE.get<SiteLocation>();
  Transform t;
  t.relativePosition.x = static_cast<float>(location.x * 32);
  t.relativePosition.y = static_cast<float>(location.y * 32);

  world.entity(name.c_str())
      .is_a(prefabE)
      .set<SiteLocation>(location)
      .set<Transform>(t)
      .child_of(constructionE.parent());

  constructionE.parent().add<ConstructionSiteNeedsUpdating>();
}

/// @brief System encapsulating the draw commands for the ConstructionSiteWindow
/// @param[in] winE The entity for the window
/// @param[in] win The Component holding the window information
void drawConstructionSiteWindow(flecs::entity winE) {
  auto &state = winE.get_mut<ConstructionSiteWindow>();
  auto world = winE.world();

  auto buildingE = state.buildingE;
  if (buildingE == flecs::entity() || !buildingE.is_alive()) {
    spdlog::error("Building is no longer valid for ConstructionSiteWindow");
    hideWindow(world, "Construction Site Window");
    return;
  }

  auto buildingPrefabs = world.lookup("Prefabs::Buildings");
  if (!buildingPrefabs.is_valid()) {
    spdlog::error("Failed to load Building Prfabs!");
    return;
  }

  // ImGui::SetNextWindowSize({170, 250}, ImGuiCond_FirstUseEver);

  auto buttonSize = ImGui::GetContentRegionAvail();
  buttonSize.y = 30;

  buildingPrefabs.children([&](flecs::entity prefabE) {
    ImGui::PushID(std::to_string(prefabE.id()).c_str());

    if (ImGui::Button(fmt::format("{}", prefabE.name().c_str()).c_str(),
                      buttonSize)) {
      buildPrefab(buildingE, prefabE);
      hideWindow(world, "Construction Site Window");
    }
    ImGui::PopID();
  });
}
