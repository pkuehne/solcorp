#include "build_mode.h"
#include <spdlog/spdlog.h>

void enterBuildMode(flecs::world &world, flecs::entity prefab) {
  if (!prefab.is_alive()) {
    spdlog::error("Cannot enter build mode for an invalid prefab");
    return;
  }
  world.set<BuildMode>({});
  world.entity<BuildMode>().add<BuildModePrefab>(prefab);
}

void exitBuildMode(flecs::world &world) {
  world.remove<BuildMode>();
  world.entity<BuildMode>().remove<BuildModePrefab>(flecs::Wildcard);
}

PendingPlacement pendingPlacement(flecs::entity prefab, const BuildMode &mode,
                                  SiteLocation hover, const Site &site) {
  const Footprint min = placementFootprint(prefab);
  if (prefab.has<FixedSize>() || !mode.dragging) {
    return {.origin = hover, .footprint = min};
  }
  const auto *limits = prefab.try_get<FootprintLimits>();
  const FootprintLimits effective{
      .min = min,
      .max = limits != nullptr ? limits->max : FootprintLimits{}.max};
  return dragRectangle(mode.drag_start, hover, effective, site);
}

flecs::entity buildModePrefab(const flecs::world &world) {
  if (!world.has<BuildMode>()) {
    return {};
  }
  return world.entity<BuildMode>().target<BuildModePrefab>();
}
