#include "modules/rocket/launch_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("LaunchInitiateRolloutAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An invalid plan") {
    LaunchInitiateRolloutAction action;

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not valid");
      }
    }
  }

  GIVEN("A plan not in Scheduled state") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    LaunchInitiateRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not in Scheduled state");
      }
    }
  }

  GIVEN("A Scheduled plan with no rocket") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    LaunchInitiateRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan has no rocket assigned");
      }
    }
  }

  GIVEN("A Scheduled plan with a rocket not in Assigned state") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    plan.add<LaunchingOn>(rocket);
    LaunchInitiateRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is not in Assigned state");
      }
    }
  }

  GIVEN("A Scheduled plan with no launchpad") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    plan.add<LaunchingOn>(rocket);
    LaunchInitiateRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan has no launchpad assigned");
      }
    }
  }

  GIVEN("A valid Scheduled plan") {
    auto storage = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(storage);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    auto launchpad = world.entity().add<Launchpad>();
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    LaunchInitiateRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      action.execute(world);

      THEN("The plan transitions to RollingOut") {
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::RollingOut")));
      }
      THEN("The rocket begins moving to the launchpad") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Moving")));
        CHECK(rocket.target<RocketTargetParent>() == launchpad);
        CHECK(rocket.target<RocketTargetState>() ==
              world.lookup("States::Rocket::Assigned"));
      }
      THEN("The rollout duration is set on the rocket") {
        // default rollout_days = 3
        REQUIRE(rocket.has<DurationRequired>());
        CHECK(rocket.get<DurationRequired>().remaining == 3);
        CHECK(rocket.get<DurationRequired>().total == 3);
      }
      THEN(
          "The rocket stays in its original location until rollout completes") {
        CHECK(rocket.parent() == storage);
      }
    }
  }
}
