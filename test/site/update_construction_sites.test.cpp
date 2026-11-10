#include "../lua/helpers/setup_helpers.h"
#include "modules/site/connector.h"
#include "modules/site/site.h"
#include "modules/site/site_construction.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>
#include <utility>
#include <vector>

namespace {
using Cell = std::pair<int, int>;
} // namespace

SCENARIO("systemUpdateConstructionSiteLocations", "[site][system]") {
  struct Case {
    std::vector<Cell> buildings;
    std::vector<Cell> connectors;
    std::vector<Cell> expected_ghosts; // sorted (x, y)
  };

  GIVEN("a 5x3 site") {
    auto [buildings, connectors, expected_ghosts] = GENERATE(values<Case>({
        {{}, {}, {{2, 1}}},                             // empty: centre cell
        {{{4, 2}}, {}, {{3, 2}, {4, 1}}},               // bottom-right corner
        {{{1, 1}}, {{2, 1}}, {{0, 1}, {1, 0}, {1, 2}}}, // not on the road
    }));

    flecs::world world;
    run_site_prefab_setup(world);
    auto site = world.entity("Site").set<Site>({.width = 5, .height = 3});
    for (auto [x, y] : buildings) {
      world.entity().child_of(site).add<Building>().set<SiteLocation>(
          {.x = static_cast<uint8_t>(x), .y = static_cast<uint8_t>(y)});
    }
    for (auto [x, y] : connectors) {
      world.entity().child_of(site).set<ConnectorTile>({}).set<SiteLocation>(
          {.x = static_cast<uint8_t>(x), .y = static_cast<uint8_t>(y)});
    }

    WHEN("its construction sites are updated") {
      Site dimensions = site.get<Site>();
      systemUpdateConstructionSiteLocations(site, dimensions);

      THEN("a ghost is on every empty cell next to a building") {
        std::vector<Cell> ghosts;
        site.children([&](flecs::entity child) {
          if (child.has<ConstructionSite>()) {
            const auto &loc = child.get<SiteLocation>();
            ghosts.emplace_back(loc.x, loc.y);
          }
        });
        std::ranges::sort(ghosts);
        CHECK(ghosts == expected_ghosts);
      }
    }
  }
}
