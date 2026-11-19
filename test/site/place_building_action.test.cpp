#include "../lua/helpers/setup_helpers.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/engine/input.h"
#include "modules/engine/render.h"
#include "modules/site/build_mode.h"
#include "modules/site/connector.h"
#include "modules/site/placement.h"
#include "modules/site/placement_actions.h"
#include "modules/site/site.h"
#include "modules/window/building_detail_window.h"
#include <SDL2/SDL.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {

struct SiteFixture {
  flecs::world world;
  flecs::entity site;
  flecs::entity factory;
  flecs::entity gate;

  SiteFixture() {
    run_site_prefab_setup(world);
    site = world.entity("Test Site")
               .set<Site>({.width = 6, .height = 6})
               .set<Transform>({})
               .add<CurrentSite>();
    for (uint8_t y = 0; y < 6; ++y) {
      world.entity().child_of(site).set<ConnectorTile>({}).set<SiteLocation>(
          {.x = 0, .y = y});
    }
    factory = world.prefab("factory")
                  .add<Building>()
                  .set<Label>({"Factory"})
                  .set<FootprintLimits>(
                      {.min = {.w = 2, .h = 2}, .max = {.w = 3, .h = 3}});
    gate = world.prefab("gate").add<Building>().add<FixedSize>();
  }

  [[nodiscard]] std::vector<flecs::entity> buildings() const {
    std::vector<flecs::entity> result;
    site.children([&](flecs::entity child) {
      if (child.has<Building>()) {
        result.push_back(child);
      }
    });
    return result;
  }
};

} // namespace

SCENARIO("PlaceBuildingAction validates where a building may go",
         "[action][site]") {
  SiteFixture f;

  struct Case {
    std::string description;
    bool fixed_size;
    SiteLocation origin;
    Footprint footprint;
    std::string expected;
  };

  GIVEN("a building to place") {
    auto [description, fixed_size, origin, footprint, expected] =
        GENERATE(values<Case>({
            {"next to the road", false, {.x = 1, .y = 1}, {.w = 2, .h = 2}, ""},
            {"a fixed-size building next to the road",
             true,
             {.x = 1, .y = 0},
             {.w = 1, .h = 1},
             ""},
            {"away from the road",
             false,
             {.x = 3, .y = 1},
             {.w = 2, .h = 2},
             "Buildings must be next to a road"},
            {"on the road",
             false,
             {.x = 0, .y = 1},
             {.w = 2, .h = 2},
             "Something is already built there"},
            {"past the site edge",
             false,
             {.x = 1, .y = 5},
             {.w = 2, .h = 2},
             "It doesn't fit on the site"},
            {"smaller than its minimum",
             false,
             {.x = 1, .y = 1},
             {.w = 1, .h = 2},
             "It is too small"},
            {"larger than its maximum",
             false,
             {.x = 1, .y = 0},
             {.w = 4, .h = 2},
             "It is too large"},
            {"a fixed-size building stretched",
             true,
             {.x = 1, .y = 0},
             {.w = 2, .h = 2},
             "This building only comes in one size"},
        }));
    CAPTURE(description);
    auto prefab = fixed_size ? f.gate : f.factory;
    PlaceBuildingAction action(PlacedPrefab{prefab}, TargetSite{f.site}, origin,
                               footprint);

    WHEN("it is validated") {
      auto result = action.validate(f.world);

      THEN("it passes or fails with the expected reason") {
        CHECK(result.ok == expected.empty());
        CHECK(result.message == expected);
      }
    }
  }

  GIVEN("no prefab is selected") {
    PlaceBuildingAction action(PlacedPrefab{}, TargetSite{f.site},
                               {.x = 1, .y = 1}, {.w = 2, .h = 2});

    THEN("it fails") {
      CHECK(action.validate(f.world).message == "No building is selected");
    }
  }
}

SCENARIO("PlaceBuildingAction places a building", "[action][site]") {
  SiteFixture f;

  GIVEN("a valid placement") {
    PlaceBuildingAction action(PlacedPrefab{f.factory}, TargetSite{f.site},
                               {.x = 1, .y = 1}, {.w = 2, .h = 2});

    WHEN("it is executed") {
      action.execute(f.world);

      THEN("one building of that prefab covers the footprint") {
        auto buildings = f.buildings();
        REQUIRE(buildings.size() == 1);
        auto building = buildings[0];
        CHECK(building.has(flecs::IsA, f.factory));
        CHECK(building.get<SiteLocation>().x == 1);
        CHECK(building.get<SiteLocation>().y == 1);
        CHECK(building.get<Footprint>().w == 2);
        CHECK(building.get<Footprint>().h == 2);
        CHECK(building.get<Transform>().relativePosition.x ==
              static_cast<float>(SITE_CELL_SIZE));
      }

      THEN("it is named after the building, numbered") {
        REQUIRE(f.buildings().size() == 1);
        CHECK(std::string{f.buildings()[0].name().c_str()} == "Factory 1");
      }

      THEN("the site is flagged for relayout") {
        CHECK(f.site.has<SiteNeedsRelayout>());
      }

      AND_WHEN("a second one is placed below it") {
        PlaceBuildingAction second(PlacedPrefab{f.factory}, TargetSite{f.site},
                                   {.x = 1, .y = 3}, {.w = 2, .h = 2});
        second.execute(f.world);

        THEN("each gets the next free number") {
          std::vector<std::string> names;
          for (auto building : f.buildings()) {
            names.emplace_back(building.name().c_str());
          }
          std::ranges::sort(names);
          CHECK(names == std::vector<std::string>{"Factory 1", "Factory 2"});
        }
      }
    }
  }

  GIVEN("an invalid placement") {
    PlaceBuildingAction action(PlacedPrefab{f.factory}, TargetSite{f.site},
                               {.x = 3, .y = 1}, {.w = 2, .h = 2});

    WHEN("it is executed") {
      action.execute(f.world);

      THEN("nothing is built") { CHECK(f.buildings().empty()); }
    }
  }
}

SCENARIO("Clicking and dragging in build mode", "[build_mode][site][system]") {
  SiteFixture f;
  auto press =
      f.world.system(f.world.lookup("SiteModule::Start Building Drag"));
  auto release =
      f.world.system(f.world.lookup("SiteModule::Place Building On Release"));
  auto cancel =
      f.world.system(f.world.lookup("SiteModule::Cancel Build On Right Click"));
  REQUIRE(press.is_valid());
  REQUIRE(release.is_valid());
  REQUIRE(cancel.is_valid());

  auto hoverOver = [&](SiteLocation cell) {
    f.world.set<HoverCell>({.location = cell});
  };
  auto pressAt = [&](SiteLocation cell, int button) {
    hoverOver(cell);
    f.world.set<MouseDown>({.x = 0, .y = 0, .button = button});
    press.run();
    f.world.remove<MouseDown>();
  };
  auto releaseAt = [&](SiteLocation cell, int button) {
    hoverOver(cell);
    f.world.set<MouseUp>({.x = 0, .y = 0, .button = button});
    release.run();
    cancel.run();
    f.world.remove<MouseUp>();
  };

  GIVEN("the factory is selected") {
    enterBuildMode(f.world, f.factory);

    WHEN("the player clicks next to the road without dragging") {
      pressAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);
      releaseAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);

      THEN("a factory of the minimum size is placed and build mode stays on") {
        auto buildings = f.buildings();
        REQUIRE(buildings.size() == 1);
        CHECK(buildings[0].get<Footprint>().w == 2);
        CHECK(buildings[0].get<Footprint>().h == 2);
        CHECK(f.world.has<BuildMode>());
        CHECK_FALSE(f.world.get<BuildMode>().dragging);
      }
    }

    WHEN("the player drags out a 3x3 rectangle") {
      pressAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);
      CHECK(f.world.get<BuildMode>().dragging);
      releaseAt({.x = 3, .y = 3}, SDL_BUTTON_LEFT);

      THEN("a factory covering that rectangle is placed") {
        auto buildings = f.buildings();
        REQUIRE(buildings.size() == 1);
        CHECK(buildings[0].get<SiteLocation>().x == 1);
        CHECK(buildings[0].get<SiteLocation>().y == 1);
        CHECK(buildings[0].get<Footprint>().w == 3);
        CHECK(buildings[0].get<Footprint>().h == 3);
      }
    }

    WHEN("the player right-clicks during a drag") {
      pressAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);
      releaseAt({.x = 3, .y = 3}, SDL_BUTTON_RIGHT);

      THEN("the drag is cancelled but build mode stays on") {
        CHECK(f.buildings().empty());
        REQUIRE(f.world.has<BuildMode>());
        CHECK_FALSE(f.world.get<BuildMode>().dragging);
      }
    }

    WHEN("the player right-clicks without dragging") {
      releaseAt({.x = 1, .y = 1}, SDL_BUTTON_RIGHT);

      THEN("build mode ends") { CHECK_FALSE(f.world.has<BuildMode>()); }
    }

    WHEN("the player releases off the site") {
      pressAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);
      f.world.remove<HoverCell>();
      f.world.set<MouseUp>({.x = 0, .y = 0, .button = SDL_BUTTON_LEFT});
      release.run();

      THEN("nothing is placed and the drag ends") {
        CHECK(f.buildings().empty());
        CHECK_FALSE(f.world.get<BuildMode>().dragging);
      }
    }

    WHEN("the player releases on an invalid rectangle") {
      pressAt({.x = 3, .y = 1}, SDL_BUTTON_LEFT);
      releaseAt({.x = 4, .y = 2}, SDL_BUTTON_LEFT);

      THEN("nothing is placed") { CHECK(f.buildings().empty()); }
    }
  }

  GIVEN("a fixed-size building is selected") {
    enterBuildMode(f.world, f.gate);

    WHEN("the player drags from next to the road") {
      pressAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);
      releaseAt({.x = 3, .y = 3}, SDL_BUTTON_LEFT);

      THEN("no drag starts, and one cell under the mouse is tried") {
        CHECK(f.buildings().empty());
      }
    }

    WHEN("the player clicks next to the road") {
      pressAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);
      releaseAt({.x = 1, .y = 1}, SDL_BUTTON_LEFT);

      THEN("a single-cell building is placed") {
        auto buildings = f.buildings();
        REQUIRE(buildings.size() == 1);
        CHECK(buildings[0].get<Footprint>().w == 1);
        CHECK(buildings[0].get<Footprint>().h == 1);
      }
    }
  }
}

SCENARIO("Building windows only open outside build mode",
         "[build_mode][site][system]") {
  SiteFixture f;
  auto match = f.world.system(
      f.world.lookup("SiteModule::Match click to Building Tile"));
  REQUIRE(match.is_valid());
  auto tileset = f.world.entity().set<Tileset>({.tile_width = SITE_CELL_SIZE,
                                                .tile_height = SITE_CELL_SIZE,
                                                .columns = 1});
  f.world.entity("Clickable")
      .child_of(f.site)
      .add<Building>()
      .set<SiteLocation>({.x = 2, .y = 2})
      .set<Tile>(tileset, {.index = 0})
      .set<Transform>(
          {.relativePosition = {}, .worldPosition = {.x = 64, .y = 64}});
  auto window = registerWindow(
                    "Building Detail", [](flecs::entity) {}, f.world)
                    .set<BuildingDetailWindow>({});
  REQUIRE(window.is_valid());

  struct Case {
    bool building;
    bool expected_open;
  };
  auto [building, expected_open] = GENERATE(values<Case>({
      {false, true},
      {true, false},
  }));
  CAPTURE(building);

  GIVEN("build mode is on or off") {
    if (building) {
      enterBuildMode(f.world, f.factory);
    }

    WHEN("the player clicks on a building") {
      f.world.set<MouseUp>({.x = 80, .y = 80, .button = SDL_BUTTON_LEFT});
      match.run();

      THEN("its window opens only outside build mode") {
        CHECK(window.get<Window>().open == expected_open);
      }
    }
  }
}
