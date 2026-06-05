#include "site.h"
#include "construction_window.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "modules/engine/engine.h"
#include "modules/engine/gui.h"
#include "modules/engine/input.h"
#include "modules/engine/render.h"
#include "modules/lua/lua.h"
#include "modules/rocket/rocket_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/site/helpers.h"
#include "modules/stats/stats.h"
#include "modules/window/building_detail_window.h"
#include "rocket_prefab_window.h"
#include "site_construction.h"
#include "weather.h"
#include <format>
#include <spdlog/spdlog.h>

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
extern unsigned char construction_png[];
extern unsigned int construction_png_len;

void systemMatchClickToBuilding(flecs::entity e, Transform &t, Sprite &s,
                                const MouseUp &mouse);

SiteModule::SiteModule(flecs::world &world) {

  world.import <BaseModule>();
  world.import <StatsModule>();
  registerEngineComponents(world);

  world.import <RocketModule>();

  // Register components
  world.component<CurrentSite>();
  world.component<ConstructionSite>();
  world.component<ConstructionSiteNeedsUpdating>();
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
      .member("available_effort", &Manufacturing::available_effort)
      .member("auto_build_next", &Manufacturing::auto_build_next)
      .member("auto_store", &Manufacturing::auto_store);
  world.component<ManufacturingLineTemplate>();
  world.component<ManufacturingLineStorage>();
  world.component<AutoBuildBlocked>();
  world.component<AutoStoreBlocked>();
  world.component<Storage>().member("max_storage", &Storage::max_storage);
  world.component<Office>().member("max_desks", &Office::max_desks);
  world.component<Launchpad>()
      .member("max_weight", &Launchpad::max_weight)
      .member("prep_days", &Launchpad::prep_days);
  world.component<BuildingDetailWindow>().member(
      "buildingE", &BuildingDetailWindow::buildingE);
  world.component<ConstructionSiteWindow>().member(
      "buildingE", &ConstructionSiteWindow::buildingE);
  world.component<RocketPrefabWindow>().member(
      "manufacturingE", &RocketPrefabWindow::manufacturingE);

  // Register Lua bindings
  register_component_lua<CurrentSite>(world, "CurrentSite");
  register_component_lua<Site>(world, "Site");
  register_component_lua<SiteLocation>(
      world, "SiteLocation", [](LuaFieldBuilder<SiteLocation> &b) {
        b.field<&SiteLocation::x>("x").field<&SiteLocation::y>("y");
      });
  register_component_lua<Launchpad>(
      world, "Launchpad", [](LuaFieldBuilder<Launchpad> &b) {
        b.nested<&Launchpad::max_weight>({"max_weight"}, {"Stat"})
            .nested<&Launchpad::prep_days>({"prep_days"}, {"Stat"});
      });
  register_component_lua<Office>(
      world, "Office", [](LuaFieldBuilder<Office> &b) {
        b.nested<&Office::max_desks>({"max_desks"}, {"Stat"});
      });
  register_component_lua<Storage>(world, "Storage");
  register_component_lua<Manufacturing>(
      world, "Manufacturing", [](LuaFieldBuilder<Manufacturing> &b) {
        b.field<&Manufacturing::max_weight>("max_weight")
            .field<&Manufacturing::available_effort>("available_effort")
            .field<&Manufacturing::auto_build_next>("auto_build_next")
            .field<&Manufacturing::auto_store>("auto_store");
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
  world.system<EffortRequired, const Manufacturing>("Update Construction")
      .term_at(1)
      .src()
      .up(flecs::ChildOf)
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemBuildingUpdateManufacuringProgress);

  world.system<const Manufacturing>("Auto Start Next Build")
      .with<ManufacturingLineTemplate>(flecs::Wildcard)
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemAutoStartNextBuild);
  world.system<Rocket, const Manufacturing>("Auto Store Built Rocket")
      .term_at(1)
      .src()
      .up(flecs::ChildOf)
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemAutoStoreBuiltRocket);

  world.system<Transform, Sprite, const MouseUp>("Match click to Building")
      .with<SiteLocation>()
      .kind(ValidatePhase)
      .each(systemMatchClickToBuilding);

  world.system<Site>("Update Construction Sites")
      .with<ConstructionSiteNeedsUpdating>()
      .kind(ValidatePhase)
      .each(systemUpdateConstructionSiteLocations);

  initWeather(world);

  world.system<const Site, const Transform>("Draw Site Border")
      .with<CurrentSite>()
      .kind(GuiPhase)
      .each([](const Site &site, const Transform &t) {
        const float x = t.worldPosition.x;
        const float y = t.worldPosition.y;
        const float w = static_cast<float>(site.width) * 32.0f;
        const float h = static_cast<float>(site.height) * 32.0f;
        const float ext = 7.0f;

        const ImU32 glow = IM_COL32(100, 180, 255, 45);
        const ImU32 border = IM_COL32(120, 195, 255, 200);
        const ImU32 cap = IM_COL32(210, 235, 255, 255);

        auto *dl = ImGui::GetBackgroundDrawList();
        dl->AddRect({x - 3, y - 3}, {x + w + 3, y + h + 3}, glow, 0, 0, 5.0f);
        dl->AddRect({x, y}, {x + w, y + h}, border, 0, 0, 2.0f);

        // Corner accents — bright L-shaped caps at each corner
        for (float cx : {x, x + w}) {
          for (float cy : {y, y + h}) {
            float sx = (cx == x) ? 1.0f : -1.0f;
            float sy = (cy == y) ? 1.0f : -1.0f;
            dl->AddLine({cx - sx * ext, cy}, {cx + sx * ext, cy}, cap, 2.0f);
            dl->AddLine({cx, cy - sy * ext}, {cx, cy + sy * ext}, cap, 2.0f);
          }
        }
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
  registerWindow("Building Detail", drawBuildingDetailWindow, world)
      .set<BuildingDetailWindow>({});
  registerWindow("Construction Site Window", drawConstructionSiteWindow, world)
      .set<ConstructionSiteWindow>({});
  registerWindow("Rocket Prefab Window", drawRocketPrefabWindow, world)
      .set<RocketPrefabWindow>({});
}

void systemMatchClickToBuilding(flecs::entity e, Transform &t, Sprite &s,
                                const MouseUp &mouse) {
  // We know from the query that this has a SiteLocation i.e. is part of a Site
  auto world = e.world();
  auto tileSize = static_cast<float>(s.width);
  auto mouse_x = static_cast<float>(mouse.x);
  auto mouse_y = static_cast<float>(mouse.y);
  if ((mouse_x > t.worldPosition.x && mouse_x < t.worldPosition.x + tileSize) &&
      (mouse_y > t.worldPosition.y && mouse_y < t.worldPosition.y + tileSize)) {
    spdlog::debug("Clicked on building {}", e.name().c_str());
    if (e.has<Building>()) {
      spdlog::debug("Showing Building Window for {}", e.name().c_str());
      showBuildingDetailWindow(e);
    } else if (e.has<ConstructionSite>()) {
      showConstructionSiteWindow(e);
    }
  }
}

void systemBuildingUpdateManufacuringProgress(
    flecs::entity e, EffortRequired &effort,
    const Manufacturing &manufacturing) {
  if (manufacturing.available_effort >= effort.remaining) {
    effort.remaining = 0;
    e.remove<EffortRequired>();
  } else {
    effort.remaining -= manufacturing.available_effort;
  }
}

void systemAutoStartNextBuild(flecs::entity manufacturingE,
                              const Manufacturing &manufacturing) {
  if (!manufacturing.auto_build_next) {
    return;
  }

  bool line_busy = false;
  manufacturingE.children([&](flecs::entity child) {
    if (child.has<Rocket>()) {
      line_busy = true;
    }
  });
  if (line_busy) {
    return;
  }

  flecs::entity prefabE = manufacturingE.target<ManufacturingLineTemplate>();
  if (!prefabE.is_valid()) {
    return;
  }

  auto world = manufacturingE.world();
  int64_t cost = computeRocketPrefabBuildCost(prefabE);
  RocketBuildAction action{PrefabEntity{prefabE}, LineEntity{manufacturingE},
                           cost};
  auto result = action.validate(world);
  if (result.ok) {
    manufacturingE.remove<AutoBuildBlocked>();
    action.execute(world);
  } else if (!manufacturingE.has<AutoBuildBlocked>()) {
    manufacturingE.add<AutoBuildBlocked>();
    instantiateBuildingNotification(world, manufacturingE, result.message);
    instantiateNotification(
        world,
        std::format("Auto-Build Blocked at {}", manufacturingE.name().c_str()),
        result.message, world.lookup("NotificationCategories::Rocket Build"),
        NotificationSeverity::Important);
  }
}

void systemAutoStoreBuiltRocket(flecs::entity rocketE, Rocket &rocket,
                                const Manufacturing &manufacturing) {
  if (!manufacturing.auto_store) {
    return;
  }
  if (!rocketE.has<RocketCurrentState>(
          rocketE.world().lookup("States::Rocket::Stored"))) {
    return;
  }

  flecs::entity lineE = rocketE.parent();
  flecs::entity storageE = lineE.target<ManufacturingLineStorage>();
  // Validity will be checked in the action validation.

  auto world = rocketE.world();
  RocketMoveAction action{RocketEntity{rocketE}, DestinationEntity{storageE},
                          static_cast<uint8_t>(rocket.move_days.value())};
  auto result = action.validate(world);
  if (result.ok) {
    lineE.remove<AutoStoreBlocked>();
    action.execute(world);
  } else if (!lineE.has<AutoStoreBlocked>()) {
    lineE.add<AutoStoreBlocked>();
    instantiateBuildingNotification(world, lineE, result.message);
    instantiateNotification(
        world, std::format("Auto-Store Blocked at {}", lineE.name().c_str()),
        result.message, world.lookup("NotificationCategories::Rocket Build"),
        NotificationSeverity::Important);
  }
}
