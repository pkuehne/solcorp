#include "helpers.h"
#include "modules/engine/movement.h"
#include "modules/engine/render.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"

flecs::entity instantiateBuilding(flecs::world &world, const std::string &name,
                                  const std::string &prefab, u_int x, u_int y,
                                  flecs::entity site) {

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
                    .set<SiteLocation>({x, y})
                    .set<Transform>(t)
                    .child_of(site);
  return entity;
};

flecs::entity instantiateConstructionSite(flecs::world &world, u_int x, u_int y,
                                          flecs::entity site) {

  std::string prefabName = "SiteModule::ConstructionSite";
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
                    .set<SiteLocation>({x, y})
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

  Text t;
  t.text = text;

  auto entity = world.entity()
                    .set<Transform>({{0, -30}, {}})
                    .set<Text>(t)
                    .set<Velocity>({0, -10})
                    .set<Expire>({1500})
                    .child_of(building);

  return entity;
}
