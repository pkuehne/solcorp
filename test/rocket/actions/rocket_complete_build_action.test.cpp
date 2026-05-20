#include "modules/base/base.h"
#include "modules/rocket/actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/window/window_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("RocketCompleteBuildAction", "[action]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <SimulationModule>();
  world.import <WindowModule>();
  world.import <RocketModule>();

  GIVEN("An invalid rocket entity") {
    RocketCompleteBuildAction complete{flecs::entity::null()};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is not valid");
      }
    }
  }

  GIVEN("A rocket that is not under construction") {
    auto rocket = world.entity().add<Rocket>();
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});
    RocketCompleteBuildAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is not under construction");
      }
    }

    WHEN("block is called") {
      complete.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        REQUIRE(rocket.get<RocketStateTransitionBlocked>().reason ==
                "Rocket is not under construction");
      }
    }
  }

  GIVEN("A rocket under construction that still has EffortRequired") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(
        world.lookup("States::Rocket::UnderConstruction"));
    rocket.set<EffortRequired>({.remaining = 50, .total = 300});
    RocketCompleteBuildAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket construction is not yet complete");
      }
    }

    WHEN("block is called") {
      complete.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        REQUIRE(rocket.get<RocketStateTransitionBlocked>().reason ==
                "Rocket construction is not yet complete");
      }
    }
  }

  GIVEN("A rocket ready to complete construction") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(
        world.lookup("States::Rocket::UnderConstruction"));
    rocket.add<RocketTargetState>(world.lookup("States::Rocket::Stored"));
    RocketCompleteBuildAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("block is called") {
      complete.block(world);

      THEN("RocketStateTransitionBlocked is not set") {
        REQUIRE(!rocket.has<RocketStateTransitionBlocked>());
      }
    }

    WHEN("Executed") {
      complete.execute(world);

      THEN("The rocket transitions to Stored and construction markers are "
           "cleared") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Stored")));
        CHECK(!rocket.has<RocketTargetState>(flecs::Wildcard));
      }
    }
  }

  GIVEN("A rocket ready to complete construction with a stale "
        "RocketStateTransitionBlocked marker") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(
        world.lookup("States::Rocket::UnderConstruction"));
    rocket.set<RocketStateTransitionBlocked>({.reason = "stale"});
    RocketCompleteBuildAction complete{rocket};

    WHEN("Executed") {
      complete.execute(world);

      THEN("RocketStateTransitionBlocked is cleared") {
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }
}
