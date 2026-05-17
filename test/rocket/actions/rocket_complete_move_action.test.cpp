#include "modules/rocket/actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/window/window_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("RocketCompleteMoveAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <WindowModule>();
  world.import <RocketModule>();

  GIVEN("An invalid rocket entity") {
    RocketCompleteMoveAction complete{flecs::entity::null()};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is not valid");
      }
    }
  }

  GIVEN("A rocket that is not currently moving") {
    auto rocket = world.entity().add<Rocket>(); // default state: Stored
    RocketCompleteMoveAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is not currently moving");
      }
    }
  }

  GIVEN("A moving rocket without a target parent") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Moving;
    RocketCompleteMoveAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket does not have a valid target");
      }
    }

    WHEN("block is called") {
      complete.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        CHECK(rocket.get<RocketStateTransitionBlocked>().reason ==
              "Rocket does not have a valid target");
      }
    }
  }

  GIVEN("A moving rocket that still has DurationRequired") {
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Moving;
    rocket.add<RocketTargetParent>(destination);
    rocket.set<DurationRequired>({.remaining = 3, .total = 5});
    RocketCompleteMoveAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket has not yet moved to the new location");
      }
    }

    WHEN("block is called") {
      complete.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        CHECK(rocket.get<RocketStateTransitionBlocked>().reason ==
              "Rocket has not yet moved to the new location");
      }
    }
  }

  GIVEN("A moving rocket with valid target parent") {
    auto source = world.entity();
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(source);
    rocket.get_mut<Rocket>().state = RocketStateId::Moving;
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});
    rocket.add<RocketTargetParent>(destination);
    RocketCompleteMoveAction complete{rocket};

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
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }

    WHEN("Executed") {
      complete.execute(world);

      THEN("The rocket is reparented and move markers are cleared") {
        CHECK(rocket.parent() == destination);
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
        CHECK(!rocket.has<RocketTargetState>());
        CHECK(!rocket.has<RocketTargetParent>());
      }
    }
  }

  GIVEN("A completed move with a stale RocketStateTransitionBlocked marker") {
    auto source = world.entity();
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(source);
    rocket.get_mut<Rocket>().state = RocketStateId::Moving;
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});
    rocket.add<RocketTargetParent>(destination);
    rocket.set<RocketStateTransitionBlocked>({.reason = "stale"});
    RocketCompleteMoveAction complete{rocket};

    WHEN("Executed") {
      complete.execute(world);

      THEN("RocketStateTransitionBlocked is cleared") {
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }
}
