#include "modules/rocket_launch/rocket_launch.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

// TEST_CASE("RocketLaunchModule loads correctly", "[rocket_launch]") {
//   flecs::world world;
//   world.import <RocketLaunchModule>();
//   REQUIRE(world.component<Rocket>().is_valid());
//   REQUIRE(world.component<CargoHold>().is_valid());
//   REQUIRE(world.component<LaunchPlan>().is_valid());
//   REQUIRE(world.component<LaunchWindow>().is_valid());
//   REQUIRE(world.component<LaunchingWith>().is_valid());
//   REQUIRE(world.component<LaunchingOn>().is_valid());
//   REQUIRE(world.component<LaunchingFrom>().is_valid());
//   REQUIRE(world.prefab<Rocket>().is_valid());
// }

SCENARIO("Rocket Launch Validation", "[rocket_launch]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

  GIVEN("An empty plan") {
    PlannedLaunch launch({});

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It is invalid") {
        REQUIRE(!result.ok);
        REQUIRE(result.message == "No rocket selected");
      }
    }
  }
  GIVEN("A valid rocket") {
    auto rocket = world.entity().is_a<Rocket>();
    PlannedLaunch launch({.rocket = rocket});
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("It still requires a launchpad") {
        REQUIRE(!result.ok);
        REQUIRE(result.message == "No launchpad selected");
      }
    }
  }
  GIVEN("A pre-allocated rocket") {
    auto rocket = world.entity().is_a<Rocket>();
    rocket.add<LaunchingOn>(world.entity());
    PlannedLaunch launch({.rocket = rocket});
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("Report the rocket is already planned") {
        REQUIRE(!result.ok);
        REQUIRE(result.message == "Rocket is already planned for a launch");
      }
    }
  }
}
