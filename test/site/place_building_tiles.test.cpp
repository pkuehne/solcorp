#include "../lua/helpers/setup_helpers.h"
#include "modules/engine/render.h"
#include "modules/site/building_tiles.h"
#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

#include <vector>

namespace {

std::vector<uint32_t> childTileIndices(flecs::entity building) {
  std::vector<uint32_t> indices;
  building.children([&](flecs::entity child) {
    if (const auto *tile = child.try_get<Tile>(flecs::Wildcard)) {
      indices.push_back(tile->index);
    }
  });
  return indices;
}

} // namespace

SCENARIO("A building's tiles are laid out when it gets a footprint",
         "[tile][site]") {
  flecs::world world;
  run_site_prefab_setup(world);
  auto tileset =
      world.entity("Tileset").set<Tileset>({.tile_width = SITE_CELL_SIZE,
                                            .tile_height = SITE_CELL_SIZE,
                                            .columns = 9});
  BuildingTiles tiles;
  for (uint32_t index = 0; index < 9; ++index) {
    tiles.tiles.push_back({.id = std::to_string(index),
                           .index = index,
                           .roles = {static_cast<TileRole>(index)},
                           .surface = TileSurface::None,
                           .weight = 1.0,
                           .predicate = {},
                           .tags = {}});
  }
  auto prefab =
      world.prefab("hall").add<Building>().set<BuildingTiles>(tileset, tiles);

  GIVEN("an instance of a tiled building") {
    auto building = world.entity().is_a(prefab);

    THEN("it has no tiles before it has a footprint") {
      CHECK(childTileIndices(building).empty());
    }

    WHEN("it is given a 3x2 footprint") {
      building.set<Footprint>({.w = 3, .h = 2});

      THEN("each cell gets the tile for its role") {
        CHECK(childTileIndices(building) ==
              std::vector<uint32_t>{0, 1, 2, 6, 7, 8});
      }

      AND_WHEN("other components change") {
        building.set<SiteLocation>({.x = 4, .y = 4});
        building.set<Transform>({});

        THEN("no extra tiles are created") {
          CHECK(childTileIndices(building).size() == 6);
        }
      }
    }
  }

  GIVEN("a building without tiles") {
    auto plain = world.entity().add<Building>();

    WHEN("it is given a footprint") {
      plain.set<Footprint>({.w = 2, .h = 2});

      THEN("no tiles are created") { CHECK(childTileIndices(plain).empty()); }
    }
  }
}
