#include "helpers.h"
#include "modules/engine/movement.h"
#include "modules/engine/render.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"

flecs::entity instantiateBuilding(flecs::world &world, const std::string &name,
                                  const std::string &prefab, uint32_t x,
                                  uint32_t y, flecs::entity site) {

  std::string prefabName = "Prefabs::Buildings::";
  prefabName.append(prefab);
  auto prefabE = world.lookup(prefabName.c_str());
  if (!prefabE.is_valid()) {
    spdlog::error("Prefab {} does not exist", prefabName);
    return flecs::entity();
  }
  if (!site.is_valid()) {
    spdlog::error("Site entity is not valid");
    return flecs::entity();
  }

  Transform t;
  t.relativePosition.x = x * 32;
  t.relativePosition.y = y * 32;

  auto entity = world.entity(name.c_str())
                    .is_a(prefabE)
                    .set<SiteLocation>({.x = x, .y = y})
                    .set<Transform>(t)
                    .child_of(site);
  return entity;
};

flecs::entity instantiateConstructionSite(flecs::world &world, uint32_t x,
                                          uint32_t y, flecs::entity site) {

  std::string prefabName = "Prefabs::Core::ConstructionSite";
  auto prefabE = world.lookup(prefabName.c_str());
  if (!prefabE.is_valid()) {
    spdlog::error("Prefab {} does not exist", prefabName);
    return flecs::entity();
  }
  if (!site.is_valid()) {
    spdlog::error("Site entity is not valid");
    return flecs::entity();
  }

  Transform t;
  t.relativePosition.x = x * 32;
  t.relativePosition.y = y * 32;

  std::string name =
      fmt::format("Construction Site {}-{}/{}", site.name().c_str(), x, y);
  auto entity = world.entity(name.c_str())
                    .is_a(prefabE)
                    .set<SiteLocation>({.x = x, .y = y})
                    .set<Transform>(t)
                    .child_of(site);
  return entity;
};

flecs::entity instantiateBuildingNotification(flecs::world &world,
                                              flecs::entity building,
                                              const std::string &text) {
  if (!building.is_valid()) {
    spdlog::error("Building entity is not valid");
    return flecs::entity();
  }

  auto entity = world.entity()
                    .set<Transform>({.relativePosition = {.x = 0, .y = -30},
                                     .worldPosition = {}})
                    .set<Text>({.text = text})
                    .set<Velocity>({.x = 0, .y = -10})
                    .set<Expire>({.millis = 1500})
                    .child_of(building);

  return entity;
}
