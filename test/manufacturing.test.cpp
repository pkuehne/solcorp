#include "modules/main/main_module.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemBuildingUpdateManufacuringProgress", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <MainModule>();
  world.import <RocketModule>();
  world.import <SiteModule>();

  flecs::entity building =
      world.entity().set<Manufacturing>({.available_effort = 20});
  auto &manuf = building.get<Manufacturing>();

  auto makeRocket = [&](uint32_t remaining, uint32_t total) {
    auto e = world.entity()
                 .add<Rocket>()
                 .set<EffortRequired>({.remaining = remaining, .total = total})
                 .set<RocketTargetState>({.target = RocketStateId::Stored})
                 .child_of(building);
    e.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    return e;
  };

  GIVEN("A rocket with more effort needed than available") {
    flecs::entity rocket = makeRocket(100, 100);
    WHEN("manufacturing update is run") {
      auto &effort = rocket.get_mut<EffortRequired>();
      systemBuildingUpdateManufacuringProgress(rocket, effort, manuf);
      THEN("remaining effort is reduced by available effort") {
        REQUIRE(rocket.get<EffortRequired>().remaining == 80);
        REQUIRE(rocket.has<EffortRequired>());
      }
    }
  }

  GIVEN("A rocket with exactly the available effort remaining") {
    flecs::entity rocket = makeRocket(20, 100);
    WHEN("manufacturing update is run") {
      auto &effort = rocket.get_mut<EffortRequired>();
      systemBuildingUpdateManufacuringProgress(rocket, effort, manuf);
      THEN("the rocket transitions to Stored") {
        REQUIRE(!rocket.has<EffortRequired>());
        REQUIRE(rocket.get<Rocket>().state == RocketStateId::Stored);
      }
    }
  }

  GIVEN("A rocket with less than the available effort remaining") {
    flecs::entity rocket = makeRocket(10, 100);
    WHEN("manufacturing update is run") {
      auto &effort = rocket.get_mut<EffortRequired>();
      systemBuildingUpdateManufacuringProgress(rocket, effort, manuf);
      THEN("the rocket transitions to Stored") {
        REQUIRE(!rocket.has<EffortRequired>());
        REQUIRE(rocket.get<Rocket>().state == RocketStateId::Stored);
      }
    }
  }

  GIVEN("A rocket with no effort remaining") {
    flecs::entity rocket = makeRocket(0, 100);
    WHEN("manufacturing update is run") {
      auto &effort = rocket.get_mut<EffortRequired>();
      systemBuildingUpdateManufacuringProgress(rocket, effort, manuf);
      THEN("the rocket transitions to Stored") {
        REQUIRE(!rocket.has<EffortRequired>());
        REQUIRE(rocket.get<Rocket>().state == RocketStateId::Stored);
      }
    }
  }
}
