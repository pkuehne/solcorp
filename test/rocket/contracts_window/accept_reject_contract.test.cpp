#include "modules/base/base.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/window/contracts_window.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("acceptContract and rejectContract update balance",
         "[contracts_window]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <SimulationModule>();
  world.import <RocketModule>();

  auto contractE =
      world.entity("TestContract")
          .set<Contract>({
              .client = "Client",
              .description = "Description",
              .upfront_payment = 500,
              .completion_payment = 1000,
              .failed = false,
          })
          .add<ContractCurrentState>(world.lookup("States::Contract::Open"));

  GIVEN("An open contract") {
    WHEN("acceptContract is called") {
      acceptContract(world, contractE);
      THEN("Status becomes Accepted and upfront payment is credited") {
        CHECK(contractE.has<ContractCurrentState>(
            world.lookup("States::Contract::Accepted")));
        CHECK(world.get<Company>().balance == 500);
      }
    }
    WHEN("rejectContract is called on an open contract") {
      rejectContract(world, contractE);
      THEN("Status becomes Closed/failed and balance is unchanged") {
        CHECK(contractE.has<ContractCurrentState>(
            world.lookup("States::Contract::Closed")));
        CHECK(contractE.get<Contract>().failed == true);
        CHECK(world.get<Company>().balance == 0);
      }
    }
  }

  GIVEN("An accepted contract") {
    acceptContract(world, contractE);
    REQUIRE(world.get<Company>().balance == 500);

    WHEN("rejectContract is called") {
      rejectContract(world, contractE);
      THEN("Upfront payment is refunded") {
        CHECK(contractE.has<ContractCurrentState>(
            world.lookup("States::Contract::Closed")));
        CHECK(contractE.get<Contract>().failed == true);
        CHECK(world.get<Company>().balance == 0);
      }
    }
  }
}
