#include "site.h"
#include "building_window.h"
#include "construction_window.h"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/engine/input.h"
#include "modules/engine/render.h"
#include "modules/lua/lua.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include "modules/site/helpers.h"
#include "modules/stats/stats.h"
#include "site_construction.h"
#include <sol/types.hpp>
#include <spdlog/spdlog.h>

extern unsigned char construction_png[];
extern unsigned int construction_png_len;

void systemMatchClickToBuilding(flecs::entity e, Transform &t, Sprite &s,
                                const MouseUp &mouse);

SiteModule::SiteModule(flecs::world &world) {

  world.import <SimulationModule>();
  world.import <RocketLaunchModule>();

  // Register components
  world.component<CurrentSite>();
  world.component<Site>()
      .member("width", &Site::width)
      .member("height", &Site::height);
  world.component<Building>();
  world.component<SiteLocation>()
      .member("x", &SiteLocation::x)
      .member("y", &SiteLocation::y);
  world.component<Facility>();
  world.component<Manufacturing>()
      .member("max_weight", &Manufacturing::max_weight)
      .member("available_effort", &Manufacturing::available_effort);
  world.component<Storage>().member("max_storage", &Storage::max_storage);
  world.component<Office>().member("max_desks", &Office::max_desks);
  world.component<Launchpad>()
      .member("max_weight", &Launchpad::max_weight)
      .member("prep_days", &Launchpad::prep_days);
  world.component<BuildingWindow>().member("buildingE",
                                           &BuildingWindow::buildingE);
  world.component<ConstructionSiteWindow>()
      .member("buildingE", &ConstructionSiteWindow::buildingE)
      .member("open", &ConstructionSiteWindow::open);

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
        userType["prep_days"] = &Launchpad::prep_days;
      });
  register_lua_user_type<Office>(world, "Office");
  register_lua_user_type<Storage>(world, "Storage");
  register_lua_user_type<Manufacturing>(
      world, "Manufacturing", [](sol::usertype<Manufacturing> &userType) {
        userType["max_weight"] = &Manufacturing::max_weight;
        userType["available_effort"] = &Manufacturing::available_effort;
      });

  // Register Systems
  world.system("Site Create Prefabs")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemCreateSitePrefabs);

  world.system("Site Create Site Windows")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemCreateSiteWindows);

  auto sim = world.get<Simulation>();
  world.system<Manufacturing>("Update Construction")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemBuildingUpdateManufacuringProgress);

  world.system<ConstructionSiteWindow>("Draw Construction Site Window")
      .kind(GuiPhase)
      .each(systemDrawConstructionSiteWindow);

  world.system<Transform, Sprite, const MouseUp>("Match click to Building")
      .with<SiteLocation>()
      .kind(ValidatePhase)
      .each(systemMatchClickToBuilding);

  world.system<Site>("Update Construction Sites")
      .with<ConstructionSiteNeedsUpdating>()
      .kind(ValidatePhase)
      .each(systemUpdateConstructionSiteLocations);

  world.system<Launchpad>()
      .kind(UpdatePhase)
      .each([](flecs::entity e, Launchpad &pad) {
        statsApplyModifiers(e, &pad.max_weight);
        statsApplyModifiers(e, &pad.prep_days);
      });
}

/**
 * @brief Registers site-related prefab entities in the ECS world.
 *
 * This function creates and registers prefab entities for buildings,
 * construction sites, and facilities, along with their associated components
 * and textures. It ensures that the necessary prefab hierarchy nodes exist,
 * loads the construction site texture, and sets up default sprite properties
 * for the prefabs.
 *
 * @param it The ECS iterator providing access to the world context.
 */
void systemCreateSitePrefabs(flecs::iter &it) {
  const flecs::world &world = it.world();

  spdlog::debug("Creating Site Prefabs");
  // Construction Site texture
  auto texture_node = world.entity("Textures");
  auto texture = world.entity("Construction")
                     .child_of(texture_node)
                     .set<Texture>(loadTexture(construction_png,
                                               construction_png_len, world));

  // Register Prefabs
  Sprite sprite;
  sprite.x = 0;
  sprite.y = 0;
  sprite.width = 32;
  sprite.height = 32;
  sprite.texture = texture;

  // Ensure prefab hierarchy nodes exist
  auto prefabs_node = world.lookup("Prefabs");
  if (!prefabs_node.is_valid()) {
    prefabs_node = world.entity("Prefabs");
  }
  auto core_node = world.lookup("Prefabs::Core");
  if (!core_node.is_valid()) {
    core_node = world.entity("Core").child_of(world.entity("Prefabs"));
  }
  auto building_node = world.lookup("Prefabs::Buildings");
  if (!building_node.is_valid()) {
    building_node = world.entity("Buildings").child_of(prefabs_node);
  }
  auto facility_node = world.lookup("Prefabs::Facilities");
  if (!facility_node.is_valid()) {
    facility_node = world.entity("Facilities").child_of(prefabs_node);
  }

  // Create Building prefab
  world.prefab("Building")
      .child_of(core_node)
      .add<Building>()
      .set<SiteLocation>({})
      .set<Transform>({})
      .set<Sprite>(sprite);

  // Create ConstructionSite prefab with adjusted sprite position
  sprite.y = 128;
  world.prefab("ConstructionSite")
      .child_of(core_node)
      .add<ConstructionSite>()
      .set<SiteLocation>({})
      .set<Transform>({})
      .set<Sprite>(sprite);

  world.prefab("Facility").child_of(core_node).add<Facility>();
}

void systemCreateSiteWindows(flecs::iter &it) {
  auto world = it.world();
  registerWindow("Building Window", drawBuildingWindow, world)
      .set<BuildingWindow>({});
}

void systemMatchClickToBuilding(flecs::entity e, Transform &t, Sprite &s,
                                const MouseUp &mouse) {
  // We know from the query that this has a SiteLocation i.e. is part of a Site
  auto world = e.world();
  int tileSize = s.width;
  if ((mouse.x > t.worldPosition.x && mouse.x < t.worldPosition.x + tileSize) &&
      (mouse.y > t.worldPosition.y && mouse.y < t.worldPosition.y + tileSize)) {
    spdlog::debug("Clicked on building {}", e.name().c_str());
    if (e.has<Building>()) {
      spdlog::debug("Showing Building Window for {}", e.name().c_str());
      showBuildingWindow(e);
    } else if (e.has<ConstructionSite>()) {
      showConstructionSiteWindow(e);
    }
  }
}

void systemBuildingUpdateManufacuringProgress(flecs::entity entity,
                                              Manufacturing &manufacturing) {
  flecs::world world = entity.world();

  flecs::entity rocket = flecs::entity::null();
  entity.children([&](flecs::entity ch) {
    if (ch.has<Construction>()) {
      rocket = ch;
    }
  });

  if (!rocket.is_valid()) {
    return;
  }
  Construction *construction = rocket.try_get_mut<Construction>();
  if (!construction) {
    return;
  }
  if (manufacturing.available_effort > construction->effort_remaining) {
    construction->effort_remaining = 0;
  } else {
    construction->effort_remaining -= manufacturing.available_effort;
  }
  if (construction->effort_remaining == 0) {
    rocket.remove<Construction>();
    instantiateBuildingNotification(world, entity, "Rocket finished");
  }
}
