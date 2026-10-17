#include "modules/lua/tileset_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <vector>

using solcorp::test::TempLuaFile;

SCENARIO("parseTilesetData reads an id-keyed map of tileset definitions",
         "[tileset][lua]") {
  struct Case {
    std::string entry;
    std::string expected_texture;
    int expected_width;
    int expected_height;
  };

  GIVEN("a tilesets.lua with a single entry") {
    auto [entry, expected_texture, expected_width, expected_height] =
        GENERATE(values<Case>({
            {R"({ texture = "roads", tile_size = 32 })", "roads", 32, 32},
            {R"({ texture = "roads", tile_width = 16, tile_height = 24 })",
             "roads", 16, 24},
            {R"({ texture = "roads", tile_size = 32, tile_height = 64 })",
             "roads", 32, 64},
            {R"({ texture = "roads" })", "roads", 0, 0},
            {R"({ tile_size = 32 })", "", 32, 32},
        }));
    CAPTURE(entry);
    TempLuaFile file("return { roads = " + entry + " }");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<TilesetDef> tilesets = parseTilesetData(lua.materialize());

      THEN("the entry is keyed by name with its texture and tile size") {
        REQUIRE(tilesets.size() == 1);
        CHECK(tilesets[0].name == "roads");
        CHECK(tilesets[0].texture == expected_texture);
        CHECK(tilesets[0].tile_width == expected_width);
        CHECK(tilesets[0].tile_height == expected_height);
      }
    }
  }

  GIVEN("an empty table") {
    TempLuaFile file("return {}");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<TilesetDef> tilesets = parseTilesetData(lua.materialize());

      THEN("no tilesets are produced") { CHECK(tilesets.empty()); }
    }
  }
}
