#include "modules/window/rocket_list_window.h"
#include "modules/base/base.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

namespace {
struct WorldFixture {
  flecs::world world;
  flecs::entity stored;
  flecs::entity invalid;

  WorldFixture() {
    world.component<StateIsTerminal>();
    world.component<RocketCurrentState>().add(flecs::Exclusive);
    world.component<Rocket>();

    auto scope = world.set_scope(0);
    auto statesRoot = world.entity("States");
    auto rocketStates = world.entity("Rocket").child_of(statesRoot);
    stored = world.entity("Stored").child_of(rocketStates);
    invalid =
        world.entity("Invalid").child_of(rocketStates).add<StateIsTerminal>();
    world.set_scope(scope);
  }

  flecs::entity makeRocket(flecs::entity state) {
    return world.entity().add<Rocket>().add<RocketCurrentState>(state);
  }
};
} // namespace

SCENARIO("rocketMatchesFilter", "[window]") {
  WorldFixture f;

  GIVEN("no state filter and hideTerminal=false") {
    RocketListWindow state{.stateFilter = flecs::entity::null(),
                           .hideTerminal = false};

    WHEN("the rocket is in a normal state") {
      auto rocket = f.makeRocket(f.stored);
      THEN("it matches") { REQUIRE(rocketMatchesFilter(rocket, state)); }
    }
    WHEN("the rocket is in a terminal state") {
      auto rocket = f.makeRocket(f.invalid);
      THEN("it matches") { REQUIRE(rocketMatchesFilter(rocket, state)); }
    }
  }

  GIVEN("no state filter and hideTerminal=true") {
    RocketListWindow state{.stateFilter = flecs::entity::null(),
                           .hideTerminal = true};

    WHEN("the rocket is in a normal state") {
      auto rocket = f.makeRocket(f.stored);
      THEN("it matches") { REQUIRE(rocketMatchesFilter(rocket, state)); }
    }
    WHEN("the rocket is in a terminal state") {
      auto rocket = f.makeRocket(f.invalid);
      THEN("it is excluded") {
        REQUIRE_FALSE(rocketMatchesFilter(rocket, state));
      }
    }
  }

  GIVEN("a state filter is set") {
    RocketListWindow state{.stateFilter = f.stored, .hideTerminal = false};

    WHEN("the rocket is in the filtered state") {
      auto rocket = f.makeRocket(f.stored);
      THEN("it matches") { REQUIRE(rocketMatchesFilter(rocket, state)); }
    }
    WHEN("the rocket is in a different state") {
      auto rocket = f.makeRocket(f.invalid);
      THEN("it is excluded") {
        REQUIRE_FALSE(rocketMatchesFilter(rocket, state));
      }
    }
  }

  GIVEN(
      "hideTerminal=true and a state filter is also set to a terminal state") {
    RocketListWindow state{.stateFilter = f.invalid, .hideTerminal = true};

    WHEN("the rocket is in that terminal state") {
      auto rocket = f.makeRocket(f.invalid);
      THEN("hideTerminal takes priority and excludes it") {
        REQUIRE_FALSE(rocketMatchesFilter(rocket, state));
      }
    }
  }
}
