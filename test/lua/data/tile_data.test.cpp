#include "modules/lua/tile_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <vector>

using solcorp::test::TempLuaFile;

SCENARIO("parseTileData reads a building's tileset and tiles", "[tile][lua]") {
  GIVEN("a tiles.lua with one building declaring two tiles") {
    TempLuaFile file(R"(
      return {
        factory = {
          tileset = "factory",
          centre_a = {
            index = 4,
            roles = { "Centre" },
            surface = "Roof",
            weight = 3,
            predicate = "powered",
            tags = { "launchpad", "lit" },
          },
          corner = { index = 0, roles = { "TopLeft", "TopRight" } },
        },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingTilesDef> buildings =
          parseTileData(lua.materialize());

      THEN("the building is keyed by id with its tileset") {
        REQUIRE(buildings.size() == 1);
        CHECK(buildings[0].building == "factory");
        CHECK(buildings[0].tileset == "factory");
      }

      THEN("every table-valued field becomes a tile with all its fields") {
        REQUIRE(buildings.size() == 1);
        const auto &tiles = buildings[0].tiles;
        REQUIRE(tiles.size() == 2);

        const TileDef &centre = tiles[0];
        CHECK(centre.id == "centre_a");
        CHECK(centre.index == 4);
        CHECK(centre.roles == std::vector<std::string>{"Centre"});
        CHECK(centre.surface == "Roof");
        CHECK(centre.weight == 3.0);
        CHECK(centre.predicate == "powered");
        CHECK(centre.tags == std::vector<std::string>{"launchpad", "lit"});

        const TileDef &corner = tiles[1];
        CHECK(corner.id == "corner");
        CHECK(corner.roles == std::vector<std::string>{"TopLeft", "TopRight"});
      }
    }
  }

  GIVEN("a tile that only declares its index") {
    TempLuaFile file(
        R"(return { silo = { tileset = "s", plain = { index = 2 } } })");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingTilesDef> buildings =
          parseTileData(lua.materialize());

      THEN("the other fields take their defaults") {
        REQUIRE(buildings.size() == 1);
        REQUIRE(buildings[0].tiles.size() == 1);
        const TileDef &tile = buildings[0].tiles[0];
        CHECK(tile.index == 2);
        CHECK(tile.roles.empty());
        CHECK(tile.surface.empty());
        CHECK(tile.weight == 1.0);
        CHECK(tile.predicate.empty());
        CHECK(tile.tags.empty());
      }
    }
  }

  GIVEN("a building entry that is not a well-formed table") {
    struct Case {
      std::string source;
      std::size_t expected_buildings;
      std::string expected_tileset;
      std::size_t expected_tiles;
    };
    auto [source, expected_buildings, expected_tileset, expected_tiles] =
        GENERATE(values<Case>({
            {R"(return { silo = "not a table" })", 0, "", 0},
            {R"(return { silo = { plain = { index = 1 } } })", 1, "", 1},
            {R"(return { silo = { tileset = "s", junk = 5 } })", 1, "s", 0},
            {R"(return { silo = { tileset = "s", roles = { "Centre" } } })", 1,
             "s", 1},
        }));
    CAPTURE(source);
    TempLuaFile file(source);
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingTilesDef> buildings =
          parseTileData(lua.materialize());

      THEN("non-table buildings and scalar fields are skipped") {
        REQUIRE(buildings.size() == expected_buildings);
        if (expected_buildings > 0) {
          CHECK(buildings[0].tileset == expected_tileset);
          CHECK(buildings[0].tiles.size() == expected_tiles);
        }
      }
    }
  }
}
