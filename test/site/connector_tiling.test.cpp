#include "modules/site/connector.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <flecs.h>
#include <map>
#include <utility>
#include <vector>

SCENARIO("rotateConnectorMaskCW", "[connector]") {
  GIVEN("A single-direction mask") {
    THEN("Each direction rotates clockwise to the next") {
      CHECK(rotateConnectorMaskCW(NeighbourNorth) == NeighbourEast);
      CHECK(rotateConnectorMaskCW(NeighbourEast) == NeighbourSouth);
      CHECK(rotateConnectorMaskCW(NeighbourSouth) == NeighbourWest);
      CHECK(rotateConnectorMaskCW(NeighbourWest) == NeighbourNorth);
    }
  }

  GIVEN("A multi-direction mask") {
    WHEN("Rotated four times") {
      uint8_t mask = NeighbourNorth | NeighbourEast;
      uint8_t rotated = mask;
      for (int i = 0; i < 4; ++i) {
        rotated = rotateConnectorMaskCW(rotated);
      }
      THEN("It returns to the original") { CHECK(rotated == mask); }
    }
  }
}

SCENARIO("computeConnectorTiling", "[connector]") {
  GIVEN("A tile with no neighbours") {
    auto tiling = computeConnectorTiling(0);
    THEN("It is left as authored (no tiling)") {
      CHECK_FALSE(tiling.has_value());
    }
  }

  GIVEN("A tile with a single neighbour") {
    WHEN("The neighbour is to the north") {
      auto tiling = computeConnectorTiling(NeighbourNorth);
      THEN("It is a dead-end facing north (rotation 270)") {
        // Matches the Lua-authored north-facing dead-end orientation.
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::DeadEnd,
                                        .rotation_deg = 270.0});
      }
    }

    WHEN("The neighbour is to the east") {
      auto tiling = computeConnectorTiling(NeighbourEast);
      THEN("It is a dead-end at the base rotation (0)") {
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::DeadEnd,
                                        .rotation_deg = 0.0});
      }
    }
  }

  GIVEN("A tile with two opposite neighbours") {
    WHEN("They run east-west") {
      auto tiling = computeConnectorTiling(NeighbourEast | NeighbourWest);
      THEN("It is a horizontal straight (rotation 0)") {
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::Straight,
                                        .rotation_deg = 0.0});
      }
    }

    WHEN("They run north-south") {
      auto tiling = computeConnectorTiling(NeighbourNorth | NeighbourSouth);
      THEN("It is a vertical straight (rotation 90)") {
        // Matches the Lua-authored vertical road orientation.
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::Straight,
                                        .rotation_deg = 90.0});
      }
    }
  }

  GIVEN("A tile with two adjacent neighbours") {
    WHEN("They are east and south (the base corner)") {
      auto tiling = computeConnectorTiling(NeighbourEast | NeighbourSouth);
      THEN("It is a corner at rotation 0") {
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::Corner,
                                        .rotation_deg = 0.0});
      }
    }

    WHEN("They are north and east") {
      auto tiling = computeConnectorTiling(NeighbourNorth | NeighbourEast);
      THEN("It is a corner rotated to face north-east (270)") {
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::Corner,
                                        .rotation_deg = 270.0});
      }
    }
  }

  GIVEN("A tile with three neighbours") {
    WHEN("The missing side is west (the base T-junction)") {
      auto tiling = computeConnectorTiling(NeighbourNorth | NeighbourEast |
                                           NeighbourSouth);
      THEN("It is a T-junction at rotation 0") {
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::TJunction,
                                        .rotation_deg = 0.0});
      }
    }

    WHEN("The missing side is north") {
      auto tiling = computeConnectorTiling(NeighbourEast | NeighbourSouth |
                                           NeighbourWest);
      THEN("It is a T-junction rotated once clockwise (90)") {
        CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::TJunction,
                                        .rotation_deg = 90.0});
      }
    }
  }

  GIVEN("A tile with four neighbours") {
    auto tiling = computeConnectorTiling(NeighbourNorth | NeighbourEast |
                                         NeighbourSouth | NeighbourWest);
    THEN("It is a crossing at rotation 0") {
      CHECK(tiling == ConnectorTiling{.variant = ConnectorVariant::Crossing,
                                      .rotation_deg = 0.0});
    }
  }
}

namespace {
using Cell = std::pair<uint8_t, uint8_t>;

// Build a site with connector tiles at the given cells (each authored as a
// plain Straight at rotation 0), run the retiler, and return each cell's
// resulting ConnectorTile keyed by position.
std::map<Cell, ConnectorTile> retileSite(const std::vector<Cell> &cells) {
  flecs::world world;
  world.component<ConnectorTile>();
  world.component<SiteLocation>();

  auto site = world.entity("Site");
  std::vector<flecs::entity> placed;
  placed.reserve(cells.size());
  for (auto [x, y] : cells) {
    placed.push_back(world.entity()
                         .child_of(site)
                         .set<SiteLocation>({.x = x, .y = y})
                         .set<ConnectorTile>({}));
  }

  systemRetileSiteConnectors(site);

  std::map<Cell, ConnectorTile> result;
  for (size_t i = 0; i < cells.size(); ++i) {
    result[cells[i]] = placed[i].get<ConnectorTile>();
  }
  return result;
}
} // namespace

SCENARIO("systemRetileSiteConnectors", "[connector]") {
  GIVEN("A vertical run of three connectors") {
    auto tiles = retileSite({{3, 0}, {3, 1}, {3, 2}});

    THEN("The endpoints are dead-ends facing inward") {
      CHECK(tiles[{3, 0}].variant == ConnectorVariant::DeadEnd);
      CHECK(tiles[{3, 0}].rotation_deg == 90.0); // opens south
      CHECK(tiles[{3, 2}].variant == ConnectorVariant::DeadEnd);
      CHECK(tiles[{3, 2}].rotation_deg == 270.0); // opens north
    }
    THEN("The middle tile is a vertical straight") {
      CHECK(tiles[{3, 1}].variant == ConnectorVariant::Straight);
      CHECK(tiles[{3, 1}].rotation_deg == 90.0);
    }
  }

  GIVEN("An L-bend") {
    auto tiles = retileSite({{0, 0}, {1, 0}, {1, 1}});

    THEN("The bend is a corner and the two ends are dead-ends") {
      CHECK(tiles[{0, 0}].variant == ConnectorVariant::DeadEnd);
      CHECK(tiles[{0, 0}].rotation_deg == 0.0); // opens east
      CHECK(tiles[{1, 0}].variant == ConnectorVariant::Corner);
      CHECK(tiles[{1, 0}].rotation_deg == 90.0); // joins west + south
      CHECK(tiles[{1, 1}].variant == ConnectorVariant::DeadEnd);
      CHECK(tiles[{1, 1}].rotation_deg == 270.0); // opens north
    }
  }

  GIVEN("A four-way cross") {
    auto tiles = retileSite({{1, 1}, {1, 0}, {0, 1}, {2, 1}, {1, 2}});

    THEN("The centre is a crossing") {
      CHECK(tiles[{1, 1}].variant == ConnectorVariant::Crossing);
      CHECK(tiles[{1, 1}].rotation_deg == 0.0);
    }
    THEN("Each arm is a dead-end pointing back at the centre") {
      CHECK(tiles[{1, 0}].variant == ConnectorVariant::DeadEnd);
      CHECK(tiles[{1, 0}].rotation_deg == 90.0); // opens south
      CHECK(tiles[{2, 1}].variant == ConnectorVariant::DeadEnd);
      CHECK(tiles[{2, 1}].rotation_deg == 180.0); // opens west
    }
  }

  GIVEN("A T-junction") {
    auto tiles = retileSite({{0, 1}, {1, 1}, {2, 1}, {1, 2}});

    THEN("The stem cell is a T-junction with its closed side facing north") {
      CHECK(tiles[{1, 1}].variant == ConnectorVariant::TJunction);
      CHECK(tiles[{1, 1}].rotation_deg == 90.0);
    }
  }

  GIVEN("A single isolated connector authored with explicit values") {
    flecs::world world;
    world.component<ConnectorTile>();
    world.component<SiteLocation>();
    auto site = world.entity("Site");
    auto tile = world.entity()
                    .child_of(site)
                    .set<SiteLocation>({.x = 5, .y = 5})
                    .set<ConnectorTile>({.variant = ConnectorVariant::Crossing,
                                         .rotation_deg = 45.0});

    WHEN("The site is retiled") {
      systemRetileSiteConnectors(site);

      THEN("The isolated tile keeps its authored variant and rotation") {
        auto result = tile.get<ConnectorTile>();
        CHECK(result.variant == ConnectorVariant::Crossing);
        CHECK(result.rotation_deg == 45.0);
      }
    }
  }
}
