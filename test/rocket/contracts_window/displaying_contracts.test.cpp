#include "modules/base/base.h"
#include "modules/rocket/contracts_window.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Displaying contracts in the ContractsWindow", "[contracts_window]") {
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
  world.entity("FailedContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .status = ContractStatus::Closed,
                      .failed = true});

  WHEN("ContractsWindow with All filter and showCompleted=true") {
    ContractsWindow state{.statusFilter = ContractFilterStatus::All,
                          .showCompleted = true};
    THEN("All contracts match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with Open filter") {
    ContractsWindow state{.statusFilter = ContractFilterStatus::Open,
                          .showCompleted = true};
    THEN("Only Open contracts match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with showCompleted=false") {
    ContractsWindow state{.statusFilter = ContractFilterStatus::All,
                          .showCompleted = false};
    THEN("Closed contracts do not match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with Closed filter") {
    ContractsWindow state{.statusFilter = ContractFilterStatus::Closed,
                          .showCompleted = true};
    THEN("Only Closed contracts match the filter") {
      REQUIRE(!contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with Accepted filter") {
    ContractsWindow state{.statusFilter = ContractFilterStatus::Accepted,
                          .showCompleted = true};
    THEN("Only Accepted contracts match the filter") {
      REQUIRE(!contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }
}
