#include "modules/rocket/launch_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("LaunchCancelAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An invalid plan") {
    LaunchCancelAction cancel;

    WHEN("Validated") {
      ValidationResult result = cancel.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not valid");
      }
    }
  }

  GIVEN("A plan already in a terminal state") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Launched"));
    LaunchCancelAction cancel{plan};

    WHEN("Validated") {
      ValidationResult result = cancel.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is already complete");
      }
    }
  }

  GIVEN("A Scheduled plan with a rocket and payloads") {
    auto storage = world.entity();
    auto launchpad = world.entity().add<Launchpad>();
    auto rocket = world.entity().add<Rocket>().child_of(storage);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    auto payload1 = world.entity().set<Payload>({500});
    auto payload2 = world.entity().set<Payload>({300});
    auto plan = world.entity("Sched Plan").set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    plan.add<LaunchingWith>(payload1);
    plan.add<LaunchingWith>(payload2);
    LaunchCancelAction cancel{plan};

    WHEN("Executed") {
      cancel.execute(world);

      THEN("The plan transitions to Cancelled") {
        CHECK(plan.is_alive());
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Cancelled")));
      }
      THEN("The rocket is returned to Stored and stays in storage") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
        CHECK(rocket.parent() == storage);
      }
      THEN("The rocket is no longer linked to the plan") {
        CHECK(!plan.has<LaunchingOn>(flecs::Wildcard));
        CHECK(!rocket.has<LaunchingOn>(flecs::Wildcard));
      }
      THEN("The payloads are released but not destroyed") {
        CHECK(payload1.is_alive());
        CHECK(payload2.is_alive());
        CHECK(!plan.has<LaunchingWith>(flecs::Wildcard));
      }
      THEN("The launchpad slot is freed") {
        CHECK(!plan.has<LaunchingFrom>(flecs::Wildcard));
      }
    }
  }

  GIVEN("A RollingOut plan where the rocket is still en route") {
    auto storage = world.entity();
    auto launchpad = world.entity().add<Launchpad>();
    auto rocket = world.entity().add<Rocket>().child_of(storage);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Moving"));
    rocket.add<RocketTargetState>(world.lookup("States::Rocket::Assigned"));
    rocket.add<RocketTargetParent>(launchpad);
    rocket.set<DurationRequired>({.remaining = 2, .total = 3});
    auto plan = world.entity("Rolling Plan").set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    LaunchCancelAction cancel{plan};

    WHEN("Executed") {
      cancel.execute(world);

      THEN("The plan transitions to Cancelled") {
        CHECK(plan.is_alive());
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Cancelled")));
      }
      THEN("The rocket move is aborted and it returns to Stored in storage") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
        CHECK(rocket.parent() == storage);
        CHECK(!rocket.has<RocketTargetParent>());
        CHECK(!rocket.has<RocketTargetState>(flecs::Wildcard));
        CHECK(!rocket.has<DurationRequired>());
      }
    }
  }

  GIVEN("An OnPad plan where the rocket is at the launchpad") {
    auto launchpad = world.entity().add<Launchpad>();
    auto rocket = world.entity().add<Rocket>().child_of(launchpad);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    auto plan = world.entity("OnPad Plan").set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    plan.set<DurationRequired>({.remaining = 3, .total = 5});
    LaunchCancelAction cancel{plan};

    WHEN("Executed") {
      cancel.execute(world);

      THEN("The plan transitions to Cancelled") {
        CHECK(plan.is_alive());
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Cancelled")));
      }
      THEN("The rocket is set to Stored but stays at the launchpad") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
        CHECK(rocket.parent() == launchpad);
      }
      THEN("The prep duration is cleared from the plan") {
        CHECK(!plan.has<DurationRequired>());
      }
    }
  }
}
