#include "setup_helpers.h"
#include "modules/lua/helpers.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("add_target_orbit_to_rocket", "[helpers][lua]") {
  flecs::world world;
  world.import<RocketModule>();
  auto rocket = world.entity("RocketLEO").add<Rocket>();
  world.entity("LEO");

  GIVEN("a rocket and an existing orbit name") {
    WHEN("add_target_orbit_to_rocket is called") {
      add_target_orbit_to_rocket(world, rocket, "LEO", 7500);
      THEN("rocket has CanLiftTo targeting the orbit with correct max_mass") {
        auto orbit = world.lookup("LEO");
        REQUIRE(rocket.has<CanLiftTo>(orbit));
        CHECK(rocket.get<CanLiftTo>(orbit).max_mass == 7500);
      }
    }
  }
}
