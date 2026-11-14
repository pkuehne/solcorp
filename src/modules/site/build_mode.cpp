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

flecs::entity buildModePrefab(const flecs::world &world) {
  if (!world.has<BuildMode>()) {
    return {};
  }
  return world.entity<BuildMode>().target<BuildModePrefab>();
}
