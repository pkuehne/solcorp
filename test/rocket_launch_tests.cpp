#include "modules/rocket_launch/actions.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Rocket Launch Validation", "[rocket_launch][action]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

  GIVEN("An empty plan") {
    PlannedLaunch launch({});

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It is invalid") {
        CHECK(!result.ok);
        CHECK(result.message == "No rocket selected");
      }
    }
  }

  GIVEN("A valid rocket") {
    auto rocket = world.entity().is_a<Rocket>();
    PlannedLaunch launch;
    launch.rocket = rocket;
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("It still requires a launchpad") {
        CHECK(!result.ok);
        CHECK(result.message == "No launchpad selected");
      }
    }
  }

  GIVEN("A pre-allocated rocket") {
    auto rocket = world.entity().is_a<Rocket>();
    rocket.add<LaunchingOn>(world.entity());
    PlannedLaunch launch;
    launch.rocket = rocket;
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("Report the rocket is already planned") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is already planned for a launch");
      }
    }
  }

  GIVEN("A missing launchpad") {
    auto rocket = world.entity().is_a<Rocket>();
    PlannedLaunch launch;
    launch.rocket = rocket;
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("Report the launchpad  is missing") {
        CHECK(!result.ok);
        CHECK(result.message == "No launchpad selected");
      }
    }
  }

  GIVEN("A missing launchpad") {
    auto rocket = world.entity().is_a<Rocket>();
    PlannedLaunch launch;
    launch.rocket = rocket;
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("Report the launchpad  is missing") {
        CHECK(!result.ok);
        CHECK(result.message == "No launchpad selected");
      }
    }
  }
  GIVEN("A valid plan") {
    auto rocket = world.entity().is_a<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    PlannedLaunch launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.rocket = rocket;
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("It succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }
}

SCENARIO("Rocket Launch Execution", "[rocket_launch][action]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

  GIVEN("A valid plan") {
    auto rocket = world.entity().is_a<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    PlannedLaunch launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.rocket = rocket;

    WHEN("Executed") {
      launch.execute(world);
      THEN("A launch plan is created") {
        REQUIRE(launch.result.is_valid());
        CHECK(launch.result.get<LaunchPlan>().launch_date ==
              static_cast<u_int>(launch.launchDay));
        CHECK(launch.result.name().c_str() == launch.name);
        CHECK(launch.result.target<LaunchingOn>() == rocket);
        CHECK(launch.result.target<LaunchingFrom>() == launchpad);
      }
    }
  }
}
