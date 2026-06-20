#include "../helpers/setup_helpers.h"
#include "modules/base/base.h"
#include "modules/lua/mod_content.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("applyRocketData creates rocket prefabs", "[mod_content][lua]") {
  flecs::world world;
  run_rocket_prefab_setup(world);

  // applyRocketData looks orbits up by name, so they must already exist.
  auto leo = world.entity("Low Orbit");
  auto polar = world.entity("Polar Orbit");

  GIVEN("a rocket definition with a cost and two target orbits") {
    RocketDef falcon;
    falcon.id = "falcon_1";
    falcon.name = "Falcon 1";
    falcon.cost = 5000000;
    falcon.target_orbits = {{"Low Orbit", 6300}, {"Polar Orbit", 5600}};

    WHEN("the definition is applied") {
      applyRocketData(world, {falcon});

      THEN("a rocket prefab is created under Prefabs::Rockets keyed by id") {
        auto prefab = world.lookup("Prefabs::Rockets::falcon_1");
        REQUIRE(prefab.is_valid());
        CHECK(prefab.has<Rocket>());
      }

      THEN("the prefab carries the display name as a Label") {
        auto prefab = world.lookup("Prefabs::Rockets::falcon_1");
        REQUIRE(prefab.is_valid());
        REQUIRE(prefab.has<Label>());
        CHECK(prefab.get<Label>().label == "Falcon 1");
      }

      THEN("the prefab's cost stat reflects the data-driven value") {
        auto prefab = world.lookup("Prefabs::Rockets::falcon_1");
        REQUIRE(prefab.is_valid());
        REQUIRE(prefab.has<Rocket>());
        CHECK(prefab.get<Rocket>().cost.base() == 5000000);
      }

      THEN("the prefab can lift to each orbit with its max mass") {
        auto prefab = world.lookup("Prefabs::Rockets::falcon_1");
        REQUIRE(prefab.is_valid());
        REQUIRE(prefab.has<CanLiftTo>(leo));
        CHECK(prefab.get<CanLiftTo>(leo).max_mass == 6300);
        REQUIRE(prefab.has<CanLiftTo>(polar));
        CHECK(prefab.get<CanLiftTo>(polar).max_mass == 5600);
      }
    }
  }

  GIVEN("the same rocket definition applied twice (a reload)") {
    RocketDef falcon;
    falcon.id = "falcon_1";
    falcon.name = "Falcon 1";
    falcon.cost = 5000000;
    falcon.target_orbits = {{"Low Orbit", 6300}};

    WHEN("the definition is applied a second time") {
      applyRocketData(world, {falcon});
      applyRocketData(world, {falcon});

      THEN("the prefab is reused, not duplicated") {
        int count = 0;
        world.lookup("Prefabs::Rockets").children([&](flecs::entity child) {
          if (child.name() == "falcon_1") {
            ++count;
          }
        });
        CHECK(count == 1);
      }
    }
  }
}
