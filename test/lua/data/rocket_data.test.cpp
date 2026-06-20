#include "modules/lua/rocket_data.h"
#include "lua/data/temp_lua_file.h"
#include "modules/lua/lua_data.h"
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

namespace {

using solcorp::test::TempLuaFile;

const RocketDef *findRocket(const std::vector<RocketDef> &rockets,
                            const std::string &id) {
  auto it = std::ranges::find_if(
      rockets, [&](const RocketDef &r) { return r.id == id; });
  return it == rockets.end() ? nullptr : &*it;
}

} // namespace

SCENARIO("parseRocketData reads rocket prefab definitions") {
  GIVEN("a rockets.lua with an id-keyed map of rockets") {
    TempLuaFile file(R"(
      return {
        falcon_1 = {
          name = "Falcon 1",
          cost = 5000000,
          orbits = {
            { target = "Sun::Earth::Low Orbit",   mass = 6300 },
            { target = "Sun::Earth::Polar Orbit", mass = 5600 },
          },
        },
        heavy = {
          name = "Falcon Heavy",
          cost = 90000000,
          orbits = {
            { target = "Sun::Earth::Low Orbit", mass = 63000 },
          },
        },
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<RocketDef> rockets = parseRocketData(lua.root());

      THEN("every rocket entry is captured keyed by its id") {
        REQUIRE(rockets.size() == 2);
        REQUIRE(findRocket(rockets, "falcon_1") != nullptr);
        REQUIRE(findRocket(rockets, "heavy") != nullptr);
      }

      THEN("a rocket's display name is parsed") {
        const RocketDef *falcon = findRocket(rockets, "falcon_1");
        REQUIRE(falcon != nullptr);
        CHECK(falcon->name == "Falcon 1");
      }

      THEN("a rocket's cost is parsed") {
        const RocketDef *falcon = findRocket(rockets, "falcon_1");
        REQUIRE(falcon != nullptr);
        CHECK(falcon->cost == 5000000);
      }

      THEN("a rocket's target orbits are parsed with their max mass") {
        const RocketDef *falcon = findRocket(rockets, "falcon_1");
        REQUIRE(falcon != nullptr);
        REQUIRE(falcon->target_orbits.size() == 2);
        CHECK(falcon->target_orbits.at("Sun::Earth::Low Orbit") == 6300);
        CHECK(falcon->target_orbits.at("Sun::Earth::Polar Orbit") == 5600);
      }
    }
  }

  GIVEN("a rocket entry without a name, cost or orbits") {
    TempLuaFile file(R"(
      return {
        pathfinder = {},
      }
    )");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<RocketDef> rockets = parseRocketData(lua.root());

      THEN("the name defaults to the id, cost to zero and no orbits produced") {
        const RocketDef *rocket = findRocket(rockets, "pathfinder");
        REQUIRE(rocket != nullptr);
        CHECK(rocket->name == "pathfinder");
        CHECK(rocket->cost == 0);
        CHECK(rocket->target_orbits.empty());
      }
    }
  }

  GIVEN("an empty table") {
    TempLuaFile file("return {}");
    LuaDataFile lua(file.path());
    REQUIRE(lua.ok());

    WHEN("the data is parsed") {
      std::vector<RocketDef> rockets = parseRocketData(lua.root());

      THEN("no rockets are produced") { REQUIRE(rockets.empty()); }
    }
  }
}
