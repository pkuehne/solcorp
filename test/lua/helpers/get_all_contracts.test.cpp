#include "modules/lua/helpers.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("get_all_contracts", "[helpers][lua]") {
  flecs::world world;
  world.import<RocketModule>();

  GIVEN("no contracts in the world") {
    WHEN("get_all_contracts is called") {
      auto result = get_all_contracts(world);
      THEN("an empty vector is returned") { CHECK(result.empty()); }
    }
  }

  GIVEN("three contracts in the world") {
    world.entity("C1").set<Contract>({.client = "Client",
                                      .description = "Desc",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});
    world.entity("C2").set<Contract>({.client = "Client",
                                      .description = "Desc",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});
    world.entity("C3").set<Contract>({.client = "Client",
                                      .description = "Desc",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});
    WHEN("get_all_contracts is called") {
      auto result = get_all_contracts(world);
      THEN("all three are returned") { CHECK(result.size() == 3); }
    }
  }
}
