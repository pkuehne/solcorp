#include "modules/rocket_launch/contracts_window.h"
#include "modules/rocket_launch/actions.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("setupLaunchForPayload creates a launch plan for a contract payload "
         "and opens the launch window with the plan loaded",
         "[contracts_window]") {
  GIVEN("an accepted contract") {
    flecs::world world;
    world.import <BaseModule>();
    world.import <RocketLaunchModule>();

    flecs::entity contract = world.entity("TestContract")
                                 .set<Contract>({
                                     "TestClient",
                                     "TestDesc",
                                     1000.0f,
                                     2000.0f,
                                     ContractStatus::Accepted,
                                     false,
                                 });
    auto targetOrbit = world.entity("LEO");
    contract.add<ContractTargetOrbit>(targetOrbit);
    auto payload = world.entity("TestPayload").set<Payload>({1000});
    contract.add<ContractPayload>(payload);
    world.progress();

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
  world.import <RocketLaunchModule>();

  // Create test contracts with different statuses
  world.entity("OpenContract")
      .set<Contract>({
          "TestClient",
          "TestDesc",
          1000.0f,
          2000.0f,
          ContractStatus::Open,
          false,
      });

  world.entity("AcceptedContract")
      .set<Contract>({
          "TestClient",
          "TestDesc",
          1000.0f,
          2000.0f,
          ContractStatus::Accepted,
          false,
      });

  world.entity("ClosedContract")
      .set<Contract>({
          "TestClient",
          "TestDesc",
          1000.0f,
          2000.0f,
          ContractStatus::Closed,
          false,
      });
  world.entity("FailedContract")
      .set<Contract>({
          "TestClient",
          "TestDesc",
          1000.0f,
          2000.0f,
          ContractStatus::Closed,
          true,
      });

  WHEN("ContractsWindow with All filter and showCompleted=true") {
    ContractsWindow state{ContractFilterStatus::All, true};
    THEN("All contracts match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with Open filter") {
    ContractsWindow state{ContractFilterStatus::Open, true};
    THEN("Only Open contracts match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with showCompleted=false") {
    ContractsWindow state{ContractFilterStatus::All, false};
    THEN("Closed contracts do not match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with Closed filter") {
    ContractsWindow state{ContractFilterStatus::Closed, true};
    THEN("Only Closed contracts match the filter") {
      REQUIRE(!contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractsWindow with Accepted filter") {
    ContractsWindow state{ContractFilterStatus::Accepted, true};
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
  world.import <RocketLaunchModule>();

  // Create test contracts with different statuses
  world.entity("OpenContract")
      .set<Contract>({
          "TestClient",
          "TestDesc",
          1000.0f,
          2000.0f,
          ContractStatus::Open,
          false,
      });

  world.entity("AcceptedContract")
      .set<Contract>({
          "TestClient",
          "TestDesc",
          1000.0f,
          2000.0f,
          ContractStatus::Accepted,
          false,
      });

  world.entity("ClosedContract")
      .set<Contract>({
          "TestClient",
          "TestDesc",
          1000.0f,
          2000.0f,
          ContractStatus::Closed,
          false,
      });

  GIVEN("An Open Contract") {
    Contract &contract = world.entity("OpenContract").get_mut<Contract>();
    THEN("Accept button is enabled") {
      REQUIRE(!acceptButtonDisabled(contract));
    }
    THEN("Reject button is enabled") {
      REQUIRE(!rejectButtonDisabled(contract));
    }
    THEN("Plan button is disabled") { REQUIRE(planButtonDisabled(contract)); }
  }

  GIVEN("An Accepted Contract") {
    Contract &contract = world.entity("AcceptedContract").get_mut<Contract>();
    THEN("Accept button is disabled") {
      REQUIRE(acceptButtonDisabled(contract));
    }
    THEN("Reject button is enabled") {
      REQUIRE(!rejectButtonDisabled(contract));
    }
    THEN("Plan button is enabled") { REQUIRE(!planButtonDisabled(contract)); }
  }

  GIVEN("A failed Closed Contract") {
    Contract &contract = world.entity("ClosedContract").get_mut<Contract>();
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
    Contract &contract = world.entity("ClosedContract").get_mut<Contract>();
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