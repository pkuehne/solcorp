#include "modules/base/base.h"
#include "modules/rocket/contracts_window.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Accept/Reject/Plan buttons enabled state", "[contracts_window]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <RocketModule>();

  world.entity("OpenContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .status = ContractStatus::Open,
                      .failed = false});
  world.entity("AcceptedContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .status = ContractStatus::Accepted,
                      .failed = false});
  world.entity("ClosedContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .status = ContractStatus::Closed,
                      .failed = false});

  GIVEN("An Open Contract") {
    auto &contract = world.entity("OpenContract").get_mut<Contract>();
    THEN("Accept button is enabled") {
      REQUIRE(!acceptButtonDisabled(contract));
    }
    THEN("Reject button is enabled") {
      REQUIRE(!rejectButtonDisabled(contract));
    }
    THEN("Plan button is disabled") { REQUIRE(planButtonDisabled(contract)); }
  }

  GIVEN("An Accepted Contract") {
    auto &contract = world.entity("AcceptedContract").get_mut<Contract>();
    THEN("Accept button is disabled") {
      REQUIRE(acceptButtonDisabled(contract));
    }
    THEN("Reject button is enabled") {
      REQUIRE(!rejectButtonDisabled(contract));
    }
    THEN("Plan button is enabled") { REQUIRE(!planButtonDisabled(contract)); }
  }

  GIVEN("A failed Closed Contract") {
    auto &contract = world.entity("ClosedContract").get_mut<Contract>();
    contract.failed = true;
    THEN("Accept button is disabled") {
      REQUIRE(acceptButtonDisabled(contract));
    }
    THEN("Reject button is disabled") {
      REQUIRE(rejectButtonDisabled(contract));
    }
    THEN("Plan button is disabled") { REQUIRE(planButtonDisabled(contract)); }
  }

  GIVEN("A successful Closed Contract") {
    auto &contract = world.entity("ClosedContract").get_mut<Contract>();
    contract.failed = false;
    THEN("Accept button is disabled") {
      REQUIRE(acceptButtonDisabled(contract));
    }
    THEN("Reject button is disabled") {
      REQUIRE(rejectButtonDisabled(contract));
    }
    THEN("Plan button is disabled") { REQUIRE(planButtonDisabled(contract)); }
  }
}
