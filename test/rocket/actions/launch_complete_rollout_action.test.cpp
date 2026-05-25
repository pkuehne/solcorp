#include "modules/rocket/launch_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("LaunchCompleteRolloutAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An invalid plan") {
    LaunchCompleteRolloutAction action;

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not valid");
      }
    }
  }

  GIVEN("A plan not in RollingOut state") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    LaunchCompleteRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not rolling out");
      }
    }
  }

  GIVEN("A RollingOut plan with no rocket") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    LaunchCompleteRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan has no rocket assigned");
      }
    }
  }

  GIVEN("A RollingOut plan where the rocket has not yet arrived at the "
        "launchpad") {
    auto storage = world.entity();
    auto launchpad = world.entity().add<Launchpad>();
    auto rocket = world.entity().add<Rocket>().child_of(storage);
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    LaunchCompleteRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket has not yet arrived at the launchpad");
      }
    }
  }

  GIVEN("A RollingOut plan with the rocket at the launchpad") {
    auto launchpad = world.entity().add<Launchpad>();
    auto rocket = world.entity().add<Rocket>().child_of(launchpad);
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    LaunchCompleteRolloutAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      action.execute(world);

      THEN("The plan transitions to OnPad") {
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::OnPad")));
      }
      THEN("A prep duration is set on the plan") {
        // default prep_days = 5
        REQUIRE(plan.has<DurationRequired>());
        CHECK(plan.get<DurationRequired>().remaining == 5);
        CHECK(plan.get<DurationRequired>().total == 5);
      }
    }
  }
}
