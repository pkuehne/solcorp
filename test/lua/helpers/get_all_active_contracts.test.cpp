#include "modules/lua/helpers.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("get_all_active_contracts", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();

  GIVEN("contracts with Open, Accepted, and Closed statuses") {
    world.entity("Open1").set<Contract>({.client = "C",
                                         .description = "D",
                                         .upfront_payment = 0,
                                         .completion_payment = 0,
                                         .failed = false})
        .add<ContractCurrentState>(world.lookup("States::Contract::Open"));
    world.entity("Open2").set<Contract>({.client = "C",
                                         .description = "D",
                                         .upfront_payment = 0,
                                         .completion_payment = 0,
                                         .failed = false})
        .add<ContractCurrentState>(world.lookup("States::Contract::Open"));
    world.entity("Accepted1")
        .set<Contract>({.client = "C",
                        .description = "D",
                        .upfront_payment = 0,
                        .completion_payment = 0,
                        .failed = false})
        .add<ContractCurrentState>(world.lookup("States::Contract::Accepted"));
    world.entity("Closed1").set<Contract>({.client = "C",
                                           .description = "D",
                                           .upfront_payment = 0,
                                           .completion_payment = 0,
                                           .failed = false})
        .add<ContractCurrentState>(world.lookup("States::Contract::Closed"));
    world.entity("Closed2").set<Contract>({.client = "C",
                                           .description = "D",
                                           .upfront_payment = 0,
                                           .completion_payment = 0,
                                           .failed = false})
        .add<ContractCurrentState>(world.lookup("States::Contract::Closed"));

    WHEN("get_all_active_contracts is called") {
      auto result = get_all_active_contracts(world);
      THEN("only Open and Accepted contracts are returned") {
        CHECK(result.size() == 3);
      }
    }
  }

  GIVEN("only closed contracts") {
    world.entity("ClosedOnly")
        .set<Contract>({.client = "C",
                        .description = "D",
                        .upfront_payment = 0,
                        .completion_payment = 0,
                        .failed = false})
        .add<ContractCurrentState>(world.lookup("States::Contract::Closed"));
    WHEN("get_all_active_contracts is called") {
      auto result = get_all_active_contracts(world);
      THEN("an empty vector is returned") { CHECK(result.empty()); }
    }
  }
}
