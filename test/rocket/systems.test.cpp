#include "modules/base/base.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/window/window_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemCreateRocketPrefabs", "[rocket][system]") {
  flecs::world world;
  world.import <RocketModule>();

  GIVEN("An empty world") {
    auto system = world.system("Create Rocket Prefabs")
                      .kind(flecs::OnStart)
                      .immediate()
                      .run(systemCreateRocketPrefabs);
    WHEN("The system is run") {
      system.run();
      THEN("The Prefabs::Rockets node is created") {
        auto node = world.lookup("Prefabs::Rockets");
        CHECK(node.is_valid());
      }
      THEN("The Prefabs::Core node contains a Rocket prefab") {
        auto node = world.lookup("Prefabs::Core::Rocket");
        CHECK(node.is_valid());
        CHECK(node.has<Rocket>());
      }
    }
  }
}

SCENARIO("systemAutoInitiateRollout", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("A Scheduled plan where today is before the rollout date") {
    uint32_t today = world.get<Game>().day;
    auto storage = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(storage);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    auto launchpad = world.entity().add<Launchpad>();
    auto plan = world.entity().set<LaunchPlan>({.rollout_date = today + 5});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);

    WHEN("The system runs") {
      systemAutoInitiateRollout(plan, plan.get_mut<LaunchPlan>());

      THEN("The plan stays in Scheduled state") {
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Scheduled")));
      }
    }
  }

  GIVEN("A Scheduled plan where today is at the rollout date") {
    uint32_t today = world.get<Game>().day;
    auto storage = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(storage);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    auto launchpad = world.entity().add<Launchpad>();
    auto plan = world.entity().set<LaunchPlan>({.rollout_date = today});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::Scheduled"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);

    WHEN("The system runs") {
      systemAutoInitiateRollout(plan, plan.get_mut<LaunchPlan>());

      THEN("The plan transitions to RollingOut") {
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::RollingOut")));
      }
      THEN("The rocket begins moving to the launchpad") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Moving")));
        CHECK(rocket.target<RocketTargetParent>() == launchpad);
      }
    }
  }
}

SCENARIO("systemAutoCompleteRollout", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

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

    WHEN("The system runs") {
      systemAutoCompleteRollout(plan, plan.get_mut<LaunchPlan>());

      THEN("The plan stays in RollingOut state") {
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::RollingOut")));
      }
    }
  }

  GIVEN("A RollingOut plan where the rocket is at the launchpad") {
    auto launchpad = world.entity().add<Launchpad>();
    auto rocket = world.entity().add<Rocket>().child_of(launchpad);
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);

    WHEN("The system runs") {
      systemAutoCompleteRollout(plan, plan.get_mut<LaunchPlan>());

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

SCENARIO("systemAutoGoForLaunch", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An OnPad plan ready to launch") {
    uint32_t today = world.get<Game>().day;
    auto targetOrbit = world.entity("LEO");
    auto launchpad = world.entity().add<Launchpad>();
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().failure_rate.setBase(0.0);
    auto plan = world.entity().set<LaunchPlan>(
        {.launch_date = today, .target_orbit = targetOrbit});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);

    WHEN("The system runs") {
      systemAutoGoForLaunch(plan, plan.get_mut<LaunchPlan>());

      THEN("The rocket is destroyed and the plan transitions to Launched") {
        CHECK(!rocket.is_alive());
        CHECK(plan.is_alive());
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Launched")));
      }
    }
  }
}

SCENARIO("systemRocketCompleteAction", "[system]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <SimulationModule>();
  world.import <WindowModule>();
  world.import <RocketModule>();

  GIVEN("A rocket under construction with effort complete") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(
        world.lookup("States::Rocket::UnderConstruction"));
    rocket.add<RocketTargetState>(world.lookup("States::Rocket::Stored"));

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket, rocket.get_mut<Rocket>());

      THEN("The rocket transitions to Stored") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }

  GIVEN("A rocket in Stored state that unexpectedly has EffortRequired") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket, rocket.get_mut<Rocket>());

      THEN("Nothing happens — the system ignores states other than "
           "UnderConstruction and Moving") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
        CHECK(rocket.has<EffortRequired>());
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }

  GIVEN("A moving rocket with duration complete") {
    auto source = world.entity();
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(source);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Moving"));
    rocket.add<RocketTargetState>(world.lookup("States::Rocket::Stored"));
    rocket.add<RocketTargetParent>(destination);

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket, rocket.get_mut<Rocket>());

      THEN("The rocket is reparented to its destination and move markers are "
           "cleared") {
        CHECK(rocket.parent() == destination);
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
        CHECK(!rocket.has<RocketTargetState>(flecs::Wildcard));
        CHECK(!rocket.has<RocketTargetParent>());
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }
}
