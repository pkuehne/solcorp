#include "../helpers/setup_helpers.h"
#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/engine/render.h"
#include "modules/lua/mod_content.h"
#include "modules/site/building_tiles.h"
#include "modules/site/placement.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>

#include <string>
#include <vector>

namespace {

constexpr int TILE_SIZE = 32;
constexpr int TILESET_SIDE = 3 * TILE_SIZE; // 3x3 tiles, indices 0..8

const std::vector<std::string> ALL_ROLES = {
    "TopLeft", "Top",        "TopRight", "Left",       "Centre",
    "Right",   "BottomLeft", "Bottom",   "BottomRight"};
const std::vector<std::string> EDGES = {"TopLeft", "Top",        "TopRight",
                                        "Left",    "Right",      "BottomLeft",
                                        "Bottom",  "BottomRight"};

flecs::entity setupWorld(flecs::world &world) {
  run_site_prefab_setup(world);
  registerEngineComponents(world);
  auto textures = world.entity("Textures");
  world.entity("hall").child_of(textures).set<Texture>(
      {.ptr = nullptr, .width = TILESET_SIDE, .height = TILESET_SIDE});
  applyTilesetData(world, {{.name = "hall",
                            .texture = "hall",
                            .tile_width = TILE_SIZE,
                            .tile_height = TILE_SIZE}});
  applyBuildingData(world,
                    {{.id = "hall", .name = "Hall"},
                     {.id = "gate", .name = "Gate", .fixed_size = true}});
  return world.lookup("Tilesets::hall");
}

std::vector<std::string> tileIds(flecs::entity prefab, flecs::entity tileset) {
  std::vector<std::string> ids;
  if (prefab.has<BuildingTiles>(tileset)) {
    for (const auto &tile : prefab.get<BuildingTiles>(tileset).tiles) {
      ids.push_back(tile.id);
    }
  }
  return ids;
}

} // namespace

SCENARIO("applyTileData attaches a building's tiles to its prefab",
         "[tile][mod_content][lua]") {
  flecs::world world;
  auto tileset = setupWorld(world);
  REQUIRE(tileset.is_valid());
  auto hall = world.lookup("Prefabs::Buildings::hall");
  REQUIRE(hall.is_valid());

  GIVEN("tiles that always fill every role") {
    BuildingTilesDef def{
        .building = "hall",
        .tileset = "hall",
        .tiles = {{.id = "base", .index = 0, .roles = ALL_ROLES},
                  {.id = "roof",
                   .index = 4,
                   .roles = {"Centre"},
                   .surface = "Roof",
                   .weight = 2.5,
                   .predicate = "powered",
                   .tags = {"lit"}}}};

    WHEN("they are applied") {
      applyTileData(world, {def});

      THEN("the prefab points at the tileset with every tile, in order") {
        CHECK(hall.target<BuildingTiles>() == tileset);
        CHECK(tileIds(hall, tileset) ==
              std::vector<std::string>{"base", "roof"});
      }

      THEN("names are converted to their typed values") {
        REQUIRE(hall.has<BuildingTiles>(tileset));
        const BuildingTile &roof = hall.get<BuildingTiles>(tileset).tiles[1];
        CHECK(roof.index == 4);
        CHECK(roof.roles == std::vector<TileRole>{TileRole::Centre});
        CHECK(roof.surface == TileSurface::Roof);
        CHECK(roof.weight == 2.5);
        CHECK(roof.predicate == "powered");
        CHECK(roof.tags == std::vector<std::string>{"lit"});
      }
    }
  }

  GIVEN("tiles that can't be attached as declared") {
    struct Case {
      std::string description;
      BuildingTilesDef def;
      std::vector<std::string> expected_ids;
    };
    auto [description, def, expected_ids] = GENERATE_REF(values<Case>({
        {"a tile past the end of the tileset is dropped",
         {.building = "hall",
          .tileset = "hall",
          .tiles = {{.id = "base", .index = 8, .roles = ALL_ROLES},
                    {.id = "past_end", .index = 9, .roles = {"Centre"}}}},
         {"base"}},
        {"dropping the only centre leaves the building without tiles",
         {.building = "hall",
          .tileset = "hall",
          .tiles = {{.id = "edges", .index = 0, .roles = EDGES},
                    {.id = "centre", .index = 9, .roles = {"Centre"}}}},
         {}},
        {"a draggable building with no unconditional centre gets no tiles",
         {.building = "hall",
          .tileset = "hall",
          .tiles = {{.id = "edges", .index = 0, .roles = EDGES},
                    {.id = "centre",
                     .index = 1,
                     .roles = {"Centre"},
                     .predicate = "powered"}}},
         {}},
        {"a missing tileset leaves the building without tiles",
         {.building = "hall",
          .tileset = "nowhere",
          .tiles = {{.id = "base", .index = 0, .roles = ALL_ROLES}}},
         {}},
    }));
    CAPTURE(description);

    WHEN("they are applied") {
      applyTileData(world, {def});

      THEN("only the expected tiles are attached") {
        CHECK(tileIds(hall, tileset) == expected_ids);
      }
    }
  }

  GIVEN("a fixed-size building with only a centre tile") {
    auto gate = world.lookup("Prefabs::Buildings::gate");
    REQUIRE(gate.is_valid());
    BuildingTilesDef def{
        .building = "gate",
        .tileset = "hall",
        .tiles = {{.id = "gate", .index = 2, .roles = {"Centre"}}}};

    WHEN("it is applied") {
      applyTileData(world, {def});

      THEN("it keeps its tiles, as it is never 9-sliced") {
        CHECK(tileIds(gate, tileset) == std::vector<std::string>{"gate"});
      }
    }
  }

  GIVEN("tiles for a building that does not exist") {
    BuildingTilesDef def{
        .building = "ghost",
        .tileset = "hall",
        .tiles = {{.id = "base", .index = 0, .roles = ALL_ROLES}}};

    WHEN("they are applied") {
      applyTileData(world, {def});

      THEN("no prefab is created for it") {
        CHECK_FALSE(world.lookup("Prefabs::Buildings::ghost").is_valid());
      }
    }
  }

  GIVEN("a building whose tiles were attached by an earlier load") {
    applyTileData(
        world, {{.building = "hall",
                 .tileset = "hall",
                 .tiles = {{.id = "base", .index = 0, .roles = ALL_ROLES}}}});
    REQUIRE(hall.has<BuildingTiles>(tileset));

    WHEN("a reload brings tiles that no longer cover every role") {
      applyTileData(world,
                    {{.building = "hall",
                      .tileset = "hall",
                      .tiles = {{.id = "edges", .index = 0, .roles = EDGES}}}});

      THEN("the stale tiles are removed") {
        CHECK_FALSE(hall.has<BuildingTiles>(flecs::Wildcard));
      }
    }
  }
}
