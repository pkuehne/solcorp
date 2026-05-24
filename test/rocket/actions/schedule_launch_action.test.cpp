#include "modules/rocket/launch_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("ScheduleLaunchAction Validation", "[validation][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An empty plan") {
    ScheduleLaunchAction launch({});

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It is invalid") {
        CHECK(!result.ok);
        CHECK(result.message == "No rocket selected");
      }
    }
  }

  GIVEN("A rocket in Stored state") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It still requires a launchpad") {
        CHECK(!result.ok);
        CHECK(result.message == "No launchpad selected");
      }
    }
  }

  GIVEN("A rocket in Assigned state") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }

  GIVEN("A rocket under construction") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(
        world.lookup("States::Rocket::UnderConstruction"));
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }

  GIVEN("A rocket already assigned to another plan") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    rocket.add<LaunchingOn>(world.entity());
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }

  GIVEN("A missing launchpad") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("Report the launchpad is missing") {
        CHECK(!result.ok);
        CHECK(result.message == "No launchpad selected");
      }
    }
  }

  GIVEN("A new plan and an existing plan with the same name") {
    world.entity("Test Plan").set<LaunchPlan>({});

    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("Report the name clash") {
        CHECK(!result.ok);
        CHECK(result.message ==
              "A Launch Plan named 'Test Plan' already exists");
      }
    }
  }

  GIVEN("A valid plan") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      launch.execute(world);

      THEN("A launch plan is created") {
        REQUIRE(launch.result.is_valid());
        CHECK(std::cmp_equal(launch.result.get<LaunchPlan>().launch_date,
                             launch.launchDay));
        CHECK(launch.result.get<LaunchPlan>().target_orbit == orbit);
        CHECK(launch.result.name().c_str() == launch.name);
        CHECK(launch.result.target<LaunchingOn>() == rocket);
        CHECK(launch.result.target<LaunchingFrom>() == launchpad);
      }
      THEN("The milestone dates are computed from stats") {
        // prep_days default = 5, rollout_days default = 3, launchDay = 10
        REQUIRE(launch.result.is_valid());
        auto &plan = launch.result.get<LaunchPlan>();
        CHECK(std::cmp_equal(plan.prep_date, 5));    // 10 - 5
        CHECK(std::cmp_equal(plan.rollout_date, 2)); // 5 - 3
      }
      THEN("The rocket state is set to Assigned") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Assigned")));
      }
      THEN("The launch plan state is set to Scheduled") {
        CHECK(launch.result.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Scheduled")));
      }
    }
  }
}
