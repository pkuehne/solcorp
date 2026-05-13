#include "modules/window/window_module.h"
#include "modules/rocket/actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("RocketCompleteBuildAction Block", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <WindowModule>();
  world.import <RocketModule>();

  GIVEN("A rocket under construction with no effort remaining") {
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});

    WHEN("block is called") {
      RocketCompleteBuildAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is not set") {
        REQUIRE(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }

  GIVEN("A rocket that is not under construction") {
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});

    WHEN("block is called") {
      RocketCompleteBuildAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        REQUIRE(rocket.get<RocketStateTransitionBlocked>().reason ==
                "Rocket is not under construction");
      }
    }
  }

  GIVEN("A rocket under construction with effort still remaining") {
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    rocket.set<EffortRequired>({.remaining = 50, .total = 300});

    WHEN("block is called") {
      RocketCompleteBuildAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        REQUIRE(rocket.get<RocketStateTransitionBlocked>().reason ==
                "Rocket construction is not yet complete");
      }
    }
  }
}
