#include "../lua/helpers/setup_helpers.h"
#include "modules/engine/input.h"
#include "modules/engine/render.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>

#include <optional>

namespace {

std::optional<std::pair<int, int>> asPair(std::optional<SiteLocation> cell) {
  if (!cell) {
    return std::nullopt;
  }
  return std::pair{cell->x, cell->y};
}

} // namespace

SCENARIO("cellAt finds the site cell under a point", "[site]") {
  struct Case {
    Point point;
    std::optional<std::pair<int, int>> expected;
  };

  GIVEN("a 4x3 site drawn at (100, 50)") {
    const Point origin{.x = 100, .y = 50};
    const Site site{.width = 4, .height = 3};
    auto [point, expected] = GENERATE(values<Case>({
        {{.x = 100, .y = 50}, std::pair{0, 0}},
        {{.x = 131.9F, .y = 81.9F}, std::pair{0, 0}},
        {{.x = 132, .y = 50}, std::pair{1, 0}},
        {{.x = 227.9F, .y = 145.9F}, std::pair{3, 2}},
        {{.x = 228, .y = 60}, std::nullopt},
        {{.x = 110, .y = 146}, std::nullopt},
        {{.x = 99.9F, .y = 60}, std::nullopt},
        {{.x = 110, .y = 49.9F}, std::nullopt},
    }));
    CAPTURE(point.x, point.y);

    THEN("the point maps to the expected cell, or none off the site") {
      CHECK(asPair(cellAt(origin, site, point)) == expected);
    }
  }
}

SCENARIO("The hover cell follows the mouse over the current site",
         "[site][system]") {
  flecs::world world;
  run_site_prefab_setup(world);
  world.entity()
      .set<Site>({.width = 4, .height = 3})
      .set<Transform>({.relativePosition = {.x = 100, .y = 50},
                       .worldPosition = {.x = 100, .y = 50}})
      .add<CurrentSite>();
  auto track = world.system(world.lookup("SiteModule::Track Hover Cell"));
  REQUIRE(track.is_valid());

  GIVEN("the mouse moves over a cell of the site") {
    world.set<MouseMove>({.x = 170, .y = 90, .over_ui = false});
    track.run();

    THEN("that cell is hovered") {
      REQUIRE(world.has<HoverCell>());
      CHECK(world.get<HoverCell>().location.x == 2);
      CHECK(world.get<HoverCell>().location.y == 1);
    }

    WHEN("the mouse moves off the site") {
      world.set<MouseMove>({.x = 10, .y = 10, .over_ui = false});
      track.run();

      THEN("no cell is hovered") { CHECK_FALSE(world.has<HoverCell>()); }
    }

    WHEN("the mouse moves onto a window over the site") {
      world.set<MouseMove>({.x = 170, .y = 90, .over_ui = true});
      track.run();

      THEN("no cell is hovered") { CHECK_FALSE(world.has<HoverCell>()); }
    }
  }
}
