#include "modules/rocket/launch_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("LaunchGoAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An invalid plan") {
    LaunchGoAction action;

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not valid");
      }
    }
  }

  GIVEN("A plan not in Prep state") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not in preparation");
      }
    }
  }

  GIVEN("A Prep plan with unfinished preparation") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::Prep"));
    plan.set<DurationRequired>({.remaining = 3, .total = 5});
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch preparation is not yet complete");
      }
    }
  }

  GIVEN("A Prep plan ready to launch") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::Prep"));
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      action.execute(world);

      THEN("The plan transitions to Launched") {
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Launched")));
      }
    }
  }
}
