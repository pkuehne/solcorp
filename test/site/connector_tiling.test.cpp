#include "modules/site/connector.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <flecs.h>
#include <map>
#include <optional>
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
  constexpr uint8_t N = NeighbourNorth;
  constexpr uint8_t E = NeighbourEast;
  constexpr uint8_t S = NeighbourSouth;
  constexpr uint8_t W = NeighbourWest;

  struct Case {
    uint8_t mask;
    std::optional<ConnectorTiling> expected;
  };

  GIVEN("Every combination of neighbours") {
    auto [mask, expected] = GENERATE_COPY(values<Case>({
        {0, std::nullopt},
        {N, ConnectorTiling{ConnectorVariant::DeadEnd, 270.0}},
        {E, ConnectorTiling{ConnectorVariant::DeadEnd, 0.0}},
        {S, ConnectorTiling{ConnectorVariant::DeadEnd, 90.0}},
        {W, ConnectorTiling{ConnectorVariant::DeadEnd, 180.0}},
        {E | W, ConnectorTiling{ConnectorVariant::Straight, 0.0}},
        {N | S, ConnectorTiling{ConnectorVariant::Straight, 90.0}},
        {E | S, ConnectorTiling{ConnectorVariant::Corner, 0.0}},
        {S | W, ConnectorTiling{ConnectorVariant::Corner, 90.0}},
        {N | W, ConnectorTiling{ConnectorVariant::Corner, 180.0}},
        {N | E, ConnectorTiling{ConnectorVariant::Corner, 270.0}},
        {N | E | S, ConnectorTiling{ConnectorVariant::TJunction, 0.0}},
        {E | S | W, ConnectorTiling{ConnectorVariant::TJunction, 90.0}},
        {N | S | W, ConnectorTiling{ConnectorVariant::TJunction, 180.0}},
        {N | E | W, ConnectorTiling{ConnectorVariant::TJunction, 270.0}},
        {N | E | S | W, ConnectorTiling{ConnectorVariant::Crossing, 0.0}},
    }));
    CAPTURE(static_cast<int>(mask));

    THEN("It picks the matching variant and rotation") {
      CHECK(computeConnectorTiling(mask) == expected);
    }

    WHEN("Bits above the four directions are also set") {
      THEN("They are ignored") {
        CHECK(computeConnectorTiling(0xF0 | mask) == expected);
      }
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
