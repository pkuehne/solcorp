#include "modules/lua/helpers.h"
#include "modules/lua/mod_content.h"
#include "setup_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_rocket", "[helpers][lua]") {
  flecs::world world;
  run_rocket_prefab_setup(world);
  create_rocket_prefab(world, "TestBooster");

  GIVEN("a valid prefab name") {
    WHEN("create_rocket is called without a parent") {
      auto rocket = create_rocket(world, RocketName{"Booster1"},
                                  RocketPrefabType{"TestBooster"});
      THEN("a valid entity with the given name is returned") {
        REQUIRE(rocket.is_valid());
        CHECK(std::string(rocket.name().c_str()) == "Booster1");
      }
    }

    WHEN("create_rocket is called with a parent entity") {
      auto parent = world.entity("LaunchPad");
      auto rocket = create_rocket(world, RocketName{"Booster2"},
                                  RocketPrefabType{"TestBooster"}, parent);
      THEN("the rocket is a child of the parent") {
        REQUIRE(rocket.is_valid());
        CHECK(rocket.parent() == parent);
      }
    }
  }

  GIVEN("a prefab name that does not exist") {
    WHEN("create_rocket is called") {
      auto rocket = create_rocket(world, RocketName{"Ghost"},
                                  RocketPrefabType{"NoSuchPrefab"});
      THEN("an invalid entity is returned") { CHECK(!rocket.is_valid()); }
    }
  }
}
