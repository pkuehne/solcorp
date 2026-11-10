#include "modules/site/connector.h"
#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Placed {
  CellContent content;
  uint8_t x;
  uint8_t y;
  Footprint footprint{};
};

std::vector<std::string> render(const OccupancyGrid &grid) {
  std::vector<std::string> rows;
  for (int y = 0; std::cmp_less(y, grid.height); ++y) {
    std::string row;
    for (int x = 0; std::cmp_less(x, grid.width); ++x) {
      switch (grid.at(x, y)) {
      case CellContent::Empty:
        row += '.';
        break;
      case CellContent::Building:
        row += 'B';
        break;
      case CellContent::Connector:
        row += 'C';
        break;
      }
    }
    rows.push_back(row);
  }
  return rows;
}

} // namespace

SCENARIO("buildOccupancyGrid", "[site][placement]") {
  struct Case {
    std::vector<Placed> placed;
    std::vector<std::string> expected;
  };

  GIVEN("a 5x3 site with buildings and connectors on it") {
    auto [placed, expected] = GENERATE(values<Case>({
        {{}, {".....", ".....", "....."}},
        {{{CellContent::Building, 4, 2}}, {".....", ".....", "....B"}},
        {{{CellContent::Building, 1, 0, {.w = 2, .h = 2}}},
         {".BB..", ".BB..", "....."}},
        {{{CellContent::Connector, 0, 1}, {CellContent::Connector, 1, 1}},
         {".....", "CC...", "....."}},
        {{{CellContent::Building, 4, 2, {.w = 2, .h = 2}}},
         {".....", ".....", "....B"}}, // clipped to the site
        {{{CellContent::Building, 0, 0}, {CellContent::Connector, 1, 0}},
         {"BC...", ".....", "....."}},
    }));

    flecs::world world;
    world.import <SiteModule>();
    auto site = world.entity("Site").set<Site>({.width = 5, .height = 3});
    for (const auto &p : placed) {
      auto e =
          world.entity().child_of(site).set<SiteLocation>({.x = p.x, .y = p.y});
      if (p.content == CellContent::Building) {
        e.add<Building>().set<Footprint>(p.footprint);
      } else {
        e.set<ConnectorTile>({});
      }
    }

    WHEN("its occupancy grid is built") {
      const OccupancyGrid grid = buildOccupancyGrid(site);

      THEN("each cell shows what occupies it") {
        CHECK(render(grid) == expected);
      }
    }
  }

  GIVEN("a site with a construction-site ghost and a building without a "
        "footprint") {
    flecs::world world;
    world.import <SiteModule>();
    auto site = world.entity("Site").set<Site>({.width = 3, .height = 1});
    world.entity().child_of(site).add<ConstructionSite>().set<SiteLocation>(
        {.x = 0, .y = 0});
    world.entity().child_of(site).add<Building>().set<SiteLocation>(
        {.x = 2, .y = 0});

    WHEN("its occupancy grid is built") {
      const OccupancyGrid grid = buildOccupancyGrid(site);

      THEN("the ghost is not an obstacle and the building fills one cell") {
        CHECK(render(grid) == std::vector<std::string>{"..B"});
      }
    }
  }
}
