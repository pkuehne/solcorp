#include "modules/rocket/actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("EditLaunchAction Validation", "[validation][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("No plan to edit") {
    EditLaunchAction edit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("It reports no plan to edit") {
        CHECK(!result.ok);
        CHECK(result.message == "No plan to edit");
      }
    }
  }

  GIVEN("The same name as another plan") {
    world.entity("Other Plan").set<LaunchPlan>({});
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(rocket);
    edit.launchDay = 10;
    edit.name = "Other Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("It reports the name clash") {
        CHECK(!result.ok);
        CHECK(result.message ==
              "A Launch Plan named 'Other Plan' already exists");
      }
    }
  }

  GIVEN("The same name as the plan being edited") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(rocket);
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }

  GIVEN("The same rocket already on this plan (Assigned state)") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(rocket);
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }

  GIVEN("A different rocket that is already Assigned") {
    auto oldRocket = world.entity().add<Rocket>();
    oldRocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto newRocket = world.entity().add<Rocket>();
    newRocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    newRocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(oldRocket);
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = newRocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }
}

SCENARIO("EditLaunchAction Execution", "[execution][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An existing plan") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    auto existing =
        world.entity("Test Plan").set<LaunchPlan>({.launch_date = 5});
    existing.add<LaunchingOn>(rocket);
    existing.add<LaunchingFrom>(launchpad);

    EditLaunchAction edit;
    edit.plan = existing;
    edit.launchDay = 20;
    edit.name = "Test Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Executed") {
      edit.execute(world);

      THEN("The old plan is replaced with the new one") {
        REQUIRE(edit.result.is_valid());
        CHECK(!existing.is_alive());
        CHECK(std::cmp_equal(edit.result.get<LaunchPlan>().launch_date, 20));
        CHECK(edit.result.target<LaunchingOn>() == rocket);
      }
      THEN("The rocket remains Assigned") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Assigned);
      }
    }
  }

  GIVEN("An existing plan with a rocket being swapped") {
    auto oldRocket = world.entity().add<Rocket>();
    oldRocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto newRocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    newRocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    auto existing = world.entity("Test Plan").set<LaunchPlan>({});
    existing.add<LaunchingOn>(oldRocket);

    EditLaunchAction edit;
    edit.plan = existing;
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = newRocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Executed") {
      edit.execute(world);

      THEN("The old rocket is returned to Stored") {
        CHECK(oldRocket.get<Rocket>().state == RocketStateId::Stored);
      }
      THEN("The new rocket is Assigned") {
        CHECK(newRocket.get<Rocket>().state == RocketStateId::Assigned);
      }
    }
  }
}
