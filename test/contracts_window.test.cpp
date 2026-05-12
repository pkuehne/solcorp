#include "modules/rocket/contracts_window.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/rocket/actions.h"
#include "modules/rocket/launch_window.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("acceptContract and rejectContract update balance",
         "[contracts_window]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <SimulationModule>();
  world.import <RocketModule>();

  auto contractE = world.entity("TestContract")
                       .set<Contract>({
                           .client = "Client",
                           .description = "Description",
                           .upfront_payment = 500,
                           .completion_payment = 1000,
                           .status = ContractStatus::Open,
                           .failed = false,
                       });

  GIVEN("An open contract") {
    WHEN("acceptContract is called") {
      acceptContract(world, contractE);
      THEN("Status becomes Accepted and upfront payment is credited") {
        CHECK(contractE.get<Contract>().status == ContractStatus::Accepted);
        CHECK(world.get<Company>().balance == 500);
      }
    }
    WHEN("rejectContract is called on an open contract") {
      rejectContract(world, contractE);
      THEN("Status becomes Closed/failed and balance is unchanged") {
        CHECK(contractE.get<Contract>().status == ContractStatus::Closed);
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
        CHECK(contractE.get<Contract>().status == ContractStatus::Closed);
        CHECK(contractE.get<Contract>().failed == true);
        CHECK(world.get<Company>().balance == 0);
      }
    }
  }
}

SCENARIO("setupLaunchForPayload creates a launch plan for a contract payload "
         "and opens the launch window with the plan loaded",
         "[contracts_window]") {
  GIVEN("an accepted contract") {
    flecs::world world;
    world.import <BaseModule>();
    world.import <RocketModule>();

    flecs::entity contract = world.entity("TestContract")
                                 .set<Contract>({
                                     .client = "TestClient",
                                     .description = "TestDesc",
                                     .upfront_payment = 1000u,
                                     .completion_payment = 2000u,
                                     .status = ContractStatus::Accepted,
                                     .failed = false,
                                 });
    auto targetOrbit = world.entity("LEO");
    contract.add<ContractTargetOrbit>(targetOrbit);
    auto payload = world.entity("TestPayload").set<Payload>({.mass = 1000});
    contract.add<ContractPayload>(payload);
    // Create the window directly instead of calling world.progress(), which
    // would run ImGui systems from any UI module without a valid ImGui context.
    registerWindow("Mission Plan", drawLaunchWindow, world).set<LaunchWindow>({});

    WHEN("setupLaunchForPayload is called with the contract payload") {
      auto plan = setupLaunchForPayload(payload);

      THEN("a launch plan is created with the payload and target orbit from "
           "the contract") {
        REQUIRE(plan.payloads[0] == payload);
        REQUIRE(plan.targetOrbit == targetOrbit);
      }
    }
  }
}

SCENARIO("Displaying contracts in the ContractsWindow", "[contracts_window]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <RocketModule>();

  // Create test contracts with different statuses
  world.entity("OpenContract")
      .set<Contract>({
          .client = "TestClient",
          .description = "TestDesc",
          .upfront_payment = 1000u,
          .completion_payment = 2000u,
          .status = ContractStatus::Open,
          .failed = false,
      });

  world.entity("AcceptedContract")
      .set<Contract>({
          .client = "TestClient",
          .description = "TestDesc",
          .upfront_payment = 1000u,
          .completion_payment = 2000u,
          .status = ContractStatus::Accepted,
          .failed = false,
      });

  world.entity("ClosedContract")
      .set<Contract>({
          .client = "TestClient",
          .description = "TestDesc",
          .upfront_payment = 1000u,
          .completion_payment = 2000u,
          .status = ContractStatus::Closed,
          .failed = false,
      });
  world.entity("FailedContract")
      .set<Contract>({
          .client = "TestClient",
          .description = "TestDesc",
          .upfront_payment = 1000u,
          .completion_payment = 2000u,
          .status = ContractStatus::Closed,
          .failed = true,
      });

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

SCENARIO("Accept/Reject/Plan buttons enabled state", "[contracts_window]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <RocketModule>();

  // Create test contracts with different statuses
  world.entity("OpenContract")
      .set<Contract>({
          .client = "TestClient",
          .description = "TestDesc",
          .upfront_payment = 1000u,
          .completion_payment = 2000u,
          .status = ContractStatus::Open,
          .failed = false,
      });

  world.entity("AcceptedContract")
      .set<Contract>({
          .client = "TestClient",
          .description = "TestDesc",
          .upfront_payment = 1000u,
          .completion_payment = 2000u,
          .status = ContractStatus::Accepted,
          .failed = false,
      });

  world.entity("ClosedContract")
      .set<Contract>({
          .client = "TestClient",
          .description = "TestDesc",
          .upfront_payment = 1000u,
          .completion_payment = 2000u,
          .status = ContractStatus::Closed,
          .failed = false,
      });

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
