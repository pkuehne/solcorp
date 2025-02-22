#include "site.h"
#include "building_window.h"
#include "construction_window.h"
#include "modules/engine/engine.h"
#include "modules/engine/input.h"
#include "modules/engine/render.h"
#include "modules/lua/lua.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include "modules/stats/stats.h"
#include "site_construction.h"
#include "spdlog/spdlog.h"
#include <sol/types.hpp>

void systemBuildingUpdateConstruction(flecs::entity, Manufacturing &);
void systemMatchClickToBuilding(flecs::entity e, Transform &t, Sprite &s,
                                const MouseUp &mouse);
void systemAddMissingTransform(flecs::entity entity, SiteLocation &location,
                               Sprite &sprite);

SiteModule::SiteModule(flecs::world &world) {

  world.import <EngineModule>();
  world.import <SimulationModule>();
  world.import <RocketLaunchModule>();

  // Register components
  world.component<CurrentSite>();
  world.component<Site>().member<u_int>("width").member<u_int>("height");
  world.component<Building>();
  world.component<SiteLocation>().member<u_int>("x").member<u_int>("y");
  world.component<Manufacturing>();
  world.component<Storage>();
  world.component<Office>();
  world.component<Launchpad>().member<Stat>("max_weight");
  world.component<BuildingWindow>()
      .member<flecs::entity>("buildingE")
      .member<bool>("open");
  world.component<ConstructionSiteWindow>()
      .member<flecs::entity>("buildingE")
      .member<bool>("open");

  // Register Lua bindings
  register_lua_user_type<CurrentSite>(world, "CurrentSite");
  register_lua_user_type<Site>(world, "Site");
  register_lua_user_type<SiteLocation>(
      world, "SiteLocation", [](sol::usertype<SiteLocation> &userType) {
        userType["x"] = &SiteLocation::x;
        userType["y"] = &SiteLocation::y;
      });
  register_lua_user_type<Launchpad>(
      world, "Launchpad", [](sol::usertype<Launchpad> &userType) {
        userType["max_weight"] = &Launchpad::max_weight;
      });
  register_lua_user_type<Office>(world, "Office");
  register_lua_user_type<Storage>(world, "Storage");
  register_lua_user_type<Manufacturing>(
      world, "Manufacturing", [](sol::usertype<Manufacturing> &userType) {
        userType["new"] =
            sol::constructors<Manufacturing(), Manufacturing(size_t)>();
      });

  // Register Systems
  auto sim = world.get<Simulation>();

  world.system<Manufacturing>("Update Construction")
      .tick_source(sim->speed)
      .kind(UpdatePhase)
      .each(systemBuildingUpdateConstruction);

  world.system<BuildingWindow>("Draw Building Window")
      .kind(GuiPhase)
      .each(systemDrawBuildingWindow);

  world.system<ConstructionSiteWindow>("Draw Construction Site Window")
      .kind(GuiPhase)
      .each(systemDrawConstructionSiteWindow);

  world.system<Transform, Sprite, const MouseUp>("Match click to Building")
      .with<SiteLocation>()
      .term_at(2)
      .singleton()
      .kind(ValidatePhase)
      .each(systemMatchClickToBuilding);

  world.system<Site>("Update Construction Sites")
      .with<ConstructionSiteNeedsUpdating>()
      .kind(ValidatePhase)
      .each(systemUpdateConstructionSiteLocations);

  world.system<SiteLocation, Sprite>("Add missing transforms")
      .without<Transform>()
      .kind(UpdatePhase)
      .each(systemAddMissingTransform);

  world.system<Launchpad>()
      .kind(UpdatePhase)
      .each([](flecs::entity e, Launchpad &pad) {
        statsApplyModifiers(e, &pad.max_weight);
      });

  // Construction Site texture
  auto scope = world.set_scope(0);
  auto texture_node = world.entity("Textures");
  auto texture =
      world.entity("Construction")
          .child_of(texture_node)
          .set<Texture>(loadTexture("textures/construction.png", world));
  world.set_scope(scope);

  // Register Prefabs
  Sprite sprite;
  sprite.x = 0;
  sprite.y = 0;
  sprite.width = 32;
  sprite.height = 32;
  sprite.texture = texture;

  world.prefab("Building")
      .add<Building>()
      .set<SiteLocation>({})
      .emplace<Sprite>(sprite);
}

void systemMatchClickToBuilding(flecs::entity e, Transform &t, Sprite &s,
                                const MouseUp &mouse) {
  // We know from the query that this has a SiteLocation i.e. is part of a Site
  auto world = e.world();
  int tileSize = s.width;
  if ((mouse.x > t.worldPosition.x && mouse.x < t.worldPosition.x + tileSize) &&
      (mouse.y > t.worldPosition.y && mouse.y < t.worldPosition.y + tileSize)) {
    if (e.has<Building>()) {
      showBuildingWindow(e);
    } else if (e.has<ConstructionSite>()) {
      showConstructionSiteWindow(e);
    }
  }
}

void systemBuildingUpdateConstruction(flecs::entity entity,
                                      Manufacturing &manufacturing) {
  flecs::world world = entity.world();

  for (flecs::entity &rocket : manufacturing.lines) {
    if (!rocket.is_valid()) {
      continue;
    }
    Construction *construction = rocket.get_mut<Construction>();
    if (!construction) {
      // Remove it from the manufacturing line
      rocket = flecs::entity();
      continue;
    }
    if (manufacturing.available_effort > construction->effort_remaining) {
      construction->effort_remaining = 0;
    } else {
      construction->effort_remaining -= manufacturing.available_effort;
    }
    if (construction->effort_remaining == 0) {
      rocket.remove<Construction>();
      rocket = flecs::entity();
    }
  }
}

void systemAddMissingTransform(flecs::entity entity, SiteLocation &location,
                               Sprite &sprite) {
  spdlog::debug("Add missing Transform for {}", entity.name().c_str());
  u_int tileSize = sprite.width;
  auto t = Transform{Point{static_cast<int>(location.x * tileSize),
                           static_cast<int>(location.y * tileSize)},
                     Point{}};
  entity.set<Transform>(t);
}
