#include "modules/lua/helpers.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_contract_payload", "[helpers][lua]") {
  flecs::world world;
  world.import <RocketModule>();
  auto contract = world.entity("ContractX")
                      .set<Contract>({.name = "ContractX",
                                      .client = "C",
                                      .description = "D",
                                      .upfront_payment = 0,
                                      .completion_payment = 0,
                                      .status = ContractStatus::Open,
                                      .failed = false});

  GIVEN("a contract, payload name, and no orbit") {
    WHEN("create_contract_payload is called with an empty orbit name") {
      auto payload = create_contract_payload(world, contract, "SatA", 500, "");
      THEN("a valid payload entity is returned") {
        REQUIRE(payload.is_valid());
        CHECK(payload.get<Payload>().mass == 500);
      }
      THEN("payload is a child of the contract") {
        CHECK(payload.parent() == contract);
      }
      THEN("contract has ContractPayload pointing to payload") {
        CHECK(contract.has<ContractPayload>(payload));
      }
      THEN("no ContractTargetOrbit is set") {
        CHECK(!contract.has<ContractTargetOrbit>());
      }
    }
  }

  GIVEN("a contract and a valid orbit name") {
    world.entity("GTO");
    WHEN("create_contract_payload is called with the orbit name") {
      create_contract_payload(world, contract, "SatB", 1200, "GTO");
      THEN("ContractTargetOrbit is set to the orbit entity") {
        auto orbit = world.lookup("GTO");
        CHECK(contract.has<ContractTargetOrbit>(orbit));
      }
    }
  }

  GIVEN("a contract and an orbit name that does not exist") {
    WHEN("create_contract_payload is called") {
      auto payload =
          create_contract_payload(world, contract, "SatC", 300, "Nonexistent");
      THEN("payload is still created") { CHECK(payload.is_valid()); }
      THEN("no ContractTargetOrbit is set") {
        CHECK(!contract.has<ContractTargetOrbit>());
      }
    }
  }

  GIVEN("a contract that already has a payload") {
    create_contract_payload(world, contract, "SatFirst", 400, "");
    WHEN("create_contract_payload is called again") {
      auto payload2 =
          create_contract_payload(world, contract, "SatSecond", 600, "");
      THEN("an invalid entity is returned") { CHECK(!payload2.is_valid()); }
      THEN("the contract still has only the original payload") {
        CHECK(contract.has<ContractPayload>(flecs::Wildcard));
        CHECK(!contract.has<ContractPayload>(world.lookup("SatSecond")));
      }
    }
  }

  GIVEN("an invalid contract entity") {
    WHEN("create_contract_payload is called") {
      auto payload = create_contract_payload(world, flecs::entity{},
                                             "SatInvalid", 100, "");
      THEN("an invalid entity is returned") { CHECK(!payload.is_valid()); }
    }
  }
}