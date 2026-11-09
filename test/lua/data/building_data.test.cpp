#include "modules/lua/building_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

namespace {

using solcorp::test::TempLuaFile;

const BuildingDef *findBuilding(const std::vector<BuildingDef> &buildings,
                                const std::string &id) {
  auto it = std::ranges::find_if(
      buildings, [&](const BuildingDef &b) { return b.id == id; });
  return it == buildings.end() ? nullptr : &*it;
}

} // namespace

SCENARIO("parseBuildingData reads building prefab definitions") {
  GIVEN("a buildings.lua with an id-keyed map of buildings") {
    TempLuaFile file(R"(
      return {
        launch_complex = {
          name = "Launch Complex",
          tile = { tileset = "buildings", index = 3 },
          facilities = {
            { name = "Launchpad A", type = "Launchpad" },
          },
        },
        factory = {
          name = "Factory",
          tile = { tileset = "buildings", index = 2 },
          facilities = {
            { name = "Line 1", type = "Manufacturing" },
            { name = "Line 2", type = "Manufacturing" },
          },
        },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingDef> buildings = parseBuildingData(lua.materialize());

      THEN("every building entry is captured keyed by its id") {
        REQUIRE(buildings.size() == 2);
        REQUIRE(findBuilding(buildings, "launch_complex") != nullptr);
        REQUIRE(findBuilding(buildings, "factory") != nullptr);
      }

      THEN("a building's name and tile are parsed") {
        const BuildingDef *launch = findBuilding(buildings, "launch_complex");
        REQUIRE(launch != nullptr);
        REQUIRE(launch->name == "Launch Complex");
        REQUIRE(launch->tileset == "buildings");
        REQUIRE(launch->tile_index == 3);
      }

      THEN("a building's facilities are parsed in order with their types") {
        const BuildingDef *factory = findBuilding(buildings, "factory");
        REQUIRE(factory != nullptr);
        REQUIRE(factory->facilities.size() == 2);
        REQUIRE(factory->facilities[0].name == "Line 1");
        REQUIRE(factory->facilities[0].type == "Manufacturing");
        REQUIRE(factory->facilities[1].name == "Line 2");
      }
    }
  }

  GIVEN("a building entry without a name") {
    TempLuaFile file(R"(
      return {
        silo = { tile = { tileset = "T", index = 1 } },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingDef> buildings = parseBuildingData(lua.materialize());

      THEN("the name defaults to the entry id") {
        const BuildingDef *silo = findBuilding(buildings, "silo");
        REQUIRE(silo != nullptr);
        REQUIRE(silo->name == "silo");
      }

      THEN("a missing facilities field yields no facilities") {
        const BuildingDef *silo = findBuilding(buildings, "silo");
        REQUIRE(silo != nullptr);
        REQUIRE(silo->facilities.empty());
      }
    }
  }

  GIVEN("a building entry without a tile") {
    TempLuaFile file(R"(
      return {
        plaza = { name = "Plaza" },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingDef> buildings = parseBuildingData(lua.materialize());

      THEN("the tileset is empty and the tile index is zero") {
        const BuildingDef *plaza = findBuilding(buildings, "plaza");
        REQUIRE(plaza != nullptr);
        REQUIRE(plaza->tileset.empty());
        REQUIRE(plaza->tile_index == 0);
      }
    }
  }

  GIVEN("an empty table") {
    TempLuaFile file("return {}");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingDef> buildings = parseBuildingData(lua.materialize());

      THEN("no buildings are produced") { REQUIRE(buildings.empty()); }
    }
  }

  GIVEN("a hidden building entry") {
    TempLuaFile file(R"(
      return {
        suppressed = { name = "Suppressed", hidden = true },
        normal = { name = "Normal" },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<BuildingDef> buildings = parseBuildingData(lua.materialize());

      THEN("the hidden flag is captured; it defaults to false otherwise") {
        REQUIRE(findBuilding(buildings, "suppressed")->hidden);
        REQUIRE_FALSE(findBuilding(buildings, "normal")->hidden);
      }
    }
  }
}

SCENARIO("validateBuildingDef rejects definitions that cannot become prefabs") {
  GIVEN("a definition whose facilities all have known types") {
    BuildingDef def;
    def.id = "factory";
    def.facilities = {{.name = "Line 1", .type = "Manufacturing"},
                      {.name = "Desk", .type = "Office"}};

    THEN("it validates") {
      REQUIRE_FALSE(validateBuildingDef(def).has_value());
    }
  }

  GIVEN("a definition with a facility of unknown type") {
    BuildingDef def;
    def.id = "weird";
    def.facilities = {{.name = "Portal", .type = "Teleporter"}};

    THEN("it is rejected with a reason naming the facility") {
      auto reason = validateBuildingDef(def);
      REQUIRE(reason.has_value());
      const std::string message = reason.value_or("");
      REQUIRE(message.find("Portal") != std::string::npos);
      REQUIRE(message.find("Teleporter") != std::string::npos);
    }
  }

  GIVEN("a definition with no facilities") {
    BuildingDef def;
    def.id = "plaza";

    THEN("it validates (facilities are optional)") {
      REQUIRE_FALSE(validateBuildingDef(def).has_value());
    }
  }
}
