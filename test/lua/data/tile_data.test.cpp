#include "modules/lua/tile_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <optional>
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

SCENARIO("validateTileDef rejects unusable tiles", "[tile][lua]") {
  struct Case {
    std::string description;
    TileDef tile;
    std::optional<std::string> expected;
  };

  GIVEN("a tile definition") {
    auto [description, tile, expected] = GENERATE(values<Case>({
        {"a minimal valid tile", {.roles = {"Centre"}}, std::nullopt},
        {"every field valid",
         {.index = 7,
          .roles = {"TopLeft", "BottomRight"},
          .surface = "SouthWall",
          .weight = 0.5,
          .predicate = "powered"},
         std::nullopt},
        {"a negative index",
         {.index = -1, .roles = {"Centre"}},
         "index must not be negative"},
        {"no roles", {}, "it declares no roles"},
        {"an unknown role",
         {.roles = {"Centre", "Middle"}},
         "unknown role 'Middle'"},
        {"a role in the wrong case",
         {.roles = {"centre"}},
         "unknown role 'centre'"},
        {"an unknown surface",
         {.roles = {"Centre"}, .surface = "Floor"},
         "unknown surface 'Floor'"},
        {"a zero weight",
         {.roles = {"Centre"}, .weight = 0.0},
         "weight must be positive"},
        {"a negative weight",
         {.roles = {"Centre"}, .weight = -2.0},
         "weight must be positive"},
    }));
    CAPTURE(description);

    WHEN("it is validated") {
      auto reason = validateTileDef(tile);

      THEN("the expected reason is reported") { CHECK(reason == expected); }
    }
  }
}

SCENARIO("missingRoles lists the roles no tile can always fill",
         "[tile][lua]") {
  const std::vector<std::string> all_roles = {
      "TopLeft", "Top",        "TopRight", "Left",       "Centre",
      "Right",   "BottomLeft", "Bottom",   "BottomRight"};
  const std::vector<std::string> edges = {"TopLeft", "Top",        "TopRight",
                                          "Left",    "Right",      "BottomLeft",
                                          "Bottom",  "BottomRight"};

  struct Case {
    std::string description;
    std::vector<TileDef> tiles;
    std::vector<TileRole> expected;
  };

  GIVEN("a building's tiles") {
    auto [description, tiles, expected] = GENERATE_REF(values<Case>({
        {"one tile filling every role", {{.roles = all_roles}}, {}},
        {"no tiles",
         {},
         {TileRole::TopLeft, TileRole::Top, TileRole::TopRight, TileRole::Left,
          TileRole::Centre, TileRole::Right, TileRole::BottomLeft,
          TileRole::Bottom, TileRole::BottomRight}},
        {"edges only", {{.roles = edges}}, {TileRole::Centre}},
        {"the only centre tile has a predicate",
         {{.roles = edges}, {.roles = {"Centre"}, .predicate = "powered"}},
         {TileRole::Centre}},
        {"a conditional centre alongside an unconditional one",
         {{.roles = edges},
          {.roles = {"Centre"}, .predicate = "powered"},
          {.roles = {"Centre"}}},
         {}},
        {"an unknown role name does not cover anything",
         {{.roles = edges}, {.roles = {"Middle"}}},
         {TileRole::Centre}},
    }));
    CAPTURE(description);

    WHEN("the missing roles are computed") {
      auto missing = missingRoles(tiles);

      THEN("exactly the uncovered roles are listed in 9-slice order") {
        CHECK(missing == expected);
      }
    }
  }
}

SCENARIO("tile role and surface names round-trip", "[tile][lua]") {
  GIVEN("each role") {
    auto role =
        GENERATE(TileRole::TopLeft, TileRole::Top, TileRole::TopRight,
                 TileRole::Left, TileRole::Centre, TileRole::Right,
                 TileRole::BottomLeft, TileRole::Bottom, TileRole::BottomRight);

    THEN("parsing its name gives the role back") {
      CHECK(parseTileRole(std::string{tileRoleName(role)}) == role);
    }
  }

  GIVEN("surface names") {
    struct Case {
      std::string name;
      std::optional<TileSurface> expected;
    };
    auto [name, expected] = GENERATE(values<Case>({
        {"", TileSurface::None},
        {"Roof", TileSurface::Roof},
        {"SouthWall", TileSurface::SouthWall},
        {"None", std::nullopt},
        {"roof", std::nullopt},
    }));
    CAPTURE(name);

    THEN("they parse to the expected surface") {
      CHECK(parseTileSurface(name) == expected);
    }
  }
}
