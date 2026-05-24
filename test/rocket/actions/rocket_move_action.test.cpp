#include "modules/rocket/rocket_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/window/window_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("RocketMoveAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <WindowModule>();
  world.import <RocketModule>();

  GIVEN("An invalid Rocket entity") {
    flecs::entity rocket = flecs::entity::null();
    flecs::entity destination = world.entity();
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message == "Rocket is not valid");
      }
    }
  }

  GIVEN("An invalid Destination") {
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    flecs::entity destination = flecs::entity::null();
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message == "Destination is not valid");
      }
    }
  }

  GIVEN("Destination is the same as current parent") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>().child_of(destination);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message ==
              "Rocket is already stored in the selected destination");
      }
    }
  }

  GIVEN("A rocket under construction") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.add<RocketCurrentState>(
        world.lookup("States::Rocket::UnderConstruction"));
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message == "Rocket must be currently stored to be moved");
      }
    }
  }

  GIVEN("A valid rocket and destination") {
    flecs::entity source = world.entity();
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>().child_of(source);
    rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 5};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      move.execute(world);

      THEN("The rocket remains in place until move completion") {
        CHECK(rocket.parent() == source);
      }
      THEN("The move target and duration are recorded") {
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Moving")));
        CHECK(rocket.target<RocketTargetParent>() == destination);
        CHECK(rocket.target<RocketTargetState>() ==
              world.lookup("States::Rocket::Stored"));
        REQUIRE(rocket.has<DurationRequired>());
        CHECK(rocket.get<DurationRequired>().remaining == 5);
        CHECK(rocket.get<DurationRequired>().total == 5);
      }
    }
  }
}
