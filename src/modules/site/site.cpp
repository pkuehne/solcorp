#include "site.h"
#include "build_mode.h"
#include "building_tiles.h"
#include "connector.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "modules/engine/engine.h"
#include "modules/engine/gui.h"
#include "modules/engine/helpers.h"
#include "modules/engine/input.h"
#include "modules/engine/render.h"
#include "modules/lua/lua.h"
#include "modules/rocket/rocket_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/site/helpers.h"
#include "modules/stats/stats.h"
#include "modules/window/building_detail_window.h"
#include "placement.h"
#include "placement_actions.h"
#include "rocket_prefab_window.h"
#include "weather.h"
#include <cmath>
#include <format>
#include <spdlog/spdlog.h>
#include <utility>

namespace {
void placeBuildingTiles(flecs::entity building, const Footprint &footprint);
void drawBuildGhost(flecs::entity site, const Transform &transform,
                    const HoverCell &hover, const BuildMode &mode);
} // namespace

SiteModule::SiteModule(flecs::world &world) {

  world.import <BaseModule>();
  world.import <StatsModule>();
  registerEngineComponents(world);

  world.import <RocketModule>();

  // Register components
  world.component<CurrentSite>();
  world.component<SiteNeedsRelayout>();
  world.component<Site>()
      .member("width", &Site::width)
      .member("height", &Site::height);
  world.component<Building>();
  world.component<Buildable>();
  world.component<FixedSize>();
  world.component<Footprint>()
      .member("w", &Footprint::w)
      .member("h", &Footprint::h);
  world.component<BuildingTiles>();
  world.component<FootprintLimits>()
      .member("min", &FootprintLimits::min)
      .member("max", &FootprintLimits::max);
  world.component<SiteLocation>()
      .member("x", &SiteLocation::x)
      .member("y", &SiteLocation::y);
  world.component<HoverCell>()
      .member("location", &HoverCell::location)
      .add(flecs::Singleton);
  world.component<PlacementKind>();
  world.component<BuildMode>()
      .member("kind", &BuildMode::kind)
      .member("drag_start", &BuildMode::drag_start)
      .member("dragging", &BuildMode::dragging)
      .add(flecs::Singleton);
  world.component<BuildModePrefab>().add(flecs::Exclusive);
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
  registerStatDef(world, {.id = "max-desks",
                          .display = "Max Desks",
                          .description =
                              "The maximum number of desks this facility can "
                              "hold"});
  registerStatDef(world,
                  {.id = "max-weight",
                   .display = "Max Weight",
                   .description = "The maximum weight the pad can support"});
  registerStatDef(world,
                  {.id = "prep-days",
                   .display = "Prep Days",
                   .description = "Number of days required to prepare a launch",
                   .higher_is_better = false});
  world.component<BuildingDetailWindow>().member(
      "buildingE", &BuildingDetailWindow::buildingE);
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

  // The retile system queries Site / SiteNeedsRelayout, registered above.
  registerConnectors(world);

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

  world.observer<const WindowSize>("Centre Sites on Resize")
      .event(flecs::OnSet)
      .each([](flecs::iter &it, size_t, const WindowSize &size) {
        it.world().each([&](const Site &site, Transform &transform) {
          transform.relativePosition =
              centredSiteOrigin(size.width, size.height, site);
        });
      });

  world.observer<const Site, Transform>("Centre New Site")
      .event(flecs::OnSet)
      .each(
          [](flecs::iter &it, size_t, const Site &site, Transform &transform) {
            const auto *size = it.world().try_get<WindowSize>();
            if (size != nullptr) {
              transform.relativePosition =
                  centredSiteOrigin(size->width, size->height, site);
            }
          });

  auto current_site = world.query_builder<const Site, const Transform>()
                          .with<CurrentSite>()
                          .build();
  world.system<const MouseMove>("Track Hover Cell")
      .kind(ValidatePhase)
      .each([current_site](flecs::iter &it, size_t, const MouseMove &mouse) {
        auto world = it.world();
        std::optional<SiteLocation> cell;
        current_site.each([&](const Site &site, const Transform &transform) {
          if (!mouse.over_ui) {
            cell = cellAt(transform.worldPosition, site,
                          {.x = static_cast<float>(mouse.x),
                           .y = static_cast<float>(mouse.y)});
          }
        });
        if (cell) {
          world.set<HoverCell>({.location = *cell});
        } else {
          world.remove<HoverCell>();
        }
      });

  world
      .system<const MouseDown, const HoverCell, BuildMode>(
          "Start Building Drag")
      .kind(ValidatePhase)
      .each([](flecs::iter &it, size_t, const MouseDown &mouse,
               const HoverCell &hover, BuildMode &mode) {
        auto prefab = buildModePrefab(it.world());
        if (mouse.button == SDL_BUTTON_LEFT &&
            dragShape(prefab) != DragShape::None) {
          mode.drag_start = hover.location;
          mode.dragging = true;
        }
      });

  world
      .system<const MouseUp, BuildMode, const HoverCell *>(
          "Place Building On Release")
      .kind(ValidatePhase)
      .each([current_site](flecs::iter &it, size_t, const MouseUp &mouse,
                           BuildMode &mode, const HoverCell *hover) {
        if (mouse.button != SDL_BUTTON_LEFT) {
          return;
        }
        const BuildMode released = mode;
        mode.dragging = false;
        if (hover == nullptr) {
          return;
        }
        auto world = it.world();
        auto site = current_site.first();
        auto prefab = buildModePrefab(world);
        const auto placement = pendingPlacement(
            prefab, released, hover->location, site.get<Site>());
        auto action = makePlacementAction(
            PlacedPrefab{prefab}, TargetSite{site}, released.kind, placement);
        if (auto result = action->validate(world); !result) {
          spdlog::debug("Cannot place {}: {}", prefab.name().c_str(),
                        result.message);
          return;
        }
        action->execute(world);
      });

  world.system<const MouseUp, BuildMode>("Cancel Build On Right Click")
      .kind(ValidatePhase)
      .each([](flecs::iter &it, size_t, const MouseUp &mouse, BuildMode &mode) {
        if (mouse.button != SDL_BUTTON_RIGHT) {
          return;
        }
        if (mode.dragging) {
          mode.dragging = false;
        } else {
          auto world = it.world();
          exitBuildMode(world);
        }
      });

  world.system<const MouseUp>("Open Clicked Building")
      .without<BuildMode>()
      .kind(ValidatePhase)
      .each([current_site](const MouseUp &mouse) {
        current_site.each([&](flecs::entity site, const Site &size,
                              const Transform &transform) {
          const auto cell = cellAt(transform.worldPosition, size,
                                   {.x = static_cast<float>(mouse.x),
                                    .y = static_cast<float>(mouse.y)});
          if (!cell) {
            return;
          }
          if (auto building = buildingAt(site, *cell); building.is_valid()) {
            showBuildingDetailWindow(building);
          }
        });
      });

  world.observer<const Footprint>("Place Building Tiles")
      .with<BuildingTiles>(flecs::Wildcard)
      .event(flecs::OnSet)
      .each(placeBuildingTiles);

  initWeather(world);

  world
      .system<const Transform, const HoverCell, const BuildMode>(
          "Draw Build Ghost")
      .with<Site>()
      .with<CurrentSite>()
      .kind(GuiPhase)
      .each(drawBuildGhost);

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
 * This function creates and registers prefab entities for buildings and
 * facilities, and ensures that the necessary prefab hierarchy nodes exist.
 *
 * @param it The ECS iterator providing access to the world context.
 */
void systemCreateSitePrefabs(flecs::iter &it) {
  const flecs::world &world = it.world();

  spdlog::debug("Creating Site Prefabs");
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
      .set<Transform>({});

  world.prefab("Facility").child_of(core_node).add<Facility>();
}

void systemCreateSiteWindows(flecs::iter &it) {
  auto world = it.world();
  registerWindow("Building Detail", drawBuildingDetailWindow, world)
      .set<BuildingDetailWindow>({});
  registerWindow("Rocket Prefab Window", drawRocketPrefabWindow, world)
      .set<RocketPrefabWindow>({});
}

std::optional<SiteLocation> cellAt(const Point &site_origin, const Site &site,
                                   const Point &point) {
  const auto x =
      static_cast<int>(std::floor((point.x - site_origin.x) / SITE_CELL_SIZE));
  const auto y =
      static_cast<int>(std::floor((point.y - site_origin.y) / SITE_CELL_SIZE));
  if (x < 0 || y < 0 || std::cmp_greater_equal(x, site.width) ||
      std::cmp_greater_equal(y, site.height)) {
    return std::nullopt;
  }
  return SiteLocation{.x = static_cast<uint8_t>(x),
                      .y = static_cast<uint8_t>(y)};
}

Point centredSiteOrigin(int window_width, int window_height, const Site &site) {
  const int site_width = site.width * SITE_CELL_SIZE;
  const int site_height = site.height * SITE_CELL_SIZE;
  return Point{.x = static_cast<float>(window_width - site_width) / 2.0F,
               .y = static_cast<float>(window_height - site_height) / 2.0F};
}

namespace {
void placeBuildingTiles(flecs::entity building, const Footprint &footprint) {
  auto tileset = building.target<BuildingTiles>();
  const auto *layout = tileset.try_get<Tileset>();
  const auto *tiles = building.try_get<BuildingTiles>(tileset);
  if (layout == nullptr || tiles == nullptr) {
    return;
  }

  const int width = footprint.w;
  const int height = footprint.h;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const auto role = tileRoleAt(x, y, width, height);
      const auto *tile =
          pickTile(eligibleTiles(*tiles, role, {}), random_double());
      if (tile == nullptr) {
        continue;
      }
      Transform transform;
      transform.relativePosition = {
          .x = static_cast<float>(x * layout->tile_width),
          .y = static_cast<float>(y * layout->tile_height)};
      building.world()
          .entity()
          .child_of(building)
          .set<Tile>(tileset, {.index = tile->index})
          .set<Transform>(transform);
    }
  }
}

void drawBuildGhost(flecs::entity site, const Transform &transform,
                    const HoverCell &hover, const BuildMode &mode) {
  auto prefab = buildModePrefab(site.world());
  if (!prefab.is_valid()) {
    return;
  }

  const auto placement =
      pendingPlacement(prefab, mode, hover.location, site.get<Site>());
  const auto validation =
      makePlacementAction(PlacedPrefab{prefab}, TargetSite{site}, mode.kind,
                          placement)
          ->validate(site.world());
  const bool valid = validation.ok;
  const ImU32 fill =
      valid ? IM_COL32(80, 220, 80, 90) : IM_COL32(230, 60, 60, 90);
  const ImU32 outline =
      valid ? IM_COL32(120, 255, 120, 230) : IM_COL32(255, 90, 90, 230);

  constexpr auto CELL = static_cast<float>(SITE_CELL_SIZE);
  const ImVec2 min{transform.worldPosition.x +
                       (static_cast<float>(placement.origin.x) * CELL),
                   transform.worldPosition.y +
                       (static_cast<float>(placement.origin.y) * CELL)};
  const ImVec2 max{min.x + (static_cast<float>(placement.footprint.w) * CELL),
                   min.y + (static_cast<float>(placement.footprint.h) * CELL)};

  auto *draw_list = ImGui::GetBackgroundDrawList();
  draw_list->AddRectFilled(min, max, fill);
  draw_list->AddRect(min, max, outline, 0, 0, 2.0F);

  const auto tooltip = ghostTooltip(prefab, mode, placement, validation);
  ImGui::BeginTooltip();
  ImGui::TextUnformatted(tooltip.title.c_str());
  ImGui::TextUnformatted(tooltip.size.c_str());
  if (!tooltip.problem.empty()) {
    ImGui::TextColored({1.0F, 0.4F, 0.4F, 1.0F}, "%s", tooltip.problem.c_str());
  }
  ImGui::TextDisabled("%s", tooltip.hint.c_str());
  ImGui::EndTooltip();
}
} // namespace

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
