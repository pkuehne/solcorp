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

  GIVEN("A valid plan with an assigned rocket") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
    auto plan = world.entity("Test Plan").set<LaunchPlan>({});
    plan.add<LaunchingOn>(rocket);
    LaunchCancelAction cancel{plan};

    WHEN("Executed") {
      cancel.execute(world);

      THEN("The plan is destroyed") { CHECK(!plan.is_alive()); }
      THEN("The rocket is returned to Stored") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
      }
    }
  }
}
