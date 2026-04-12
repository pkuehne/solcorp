#include "modules/rocket_launch/contracts_window.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

TEST_CASE("contractMatchesFilter filters by status", "[contracts_window]") {
  flecs::world world;
  world.import<RocketLaunchModule>();

  // Create test contracts with different statuses
  flecs::entity openContract = world.entity("OpenContract")
                                   .set<Contract>({
                                       "TestClient",
                                       "TestDesc",
                                       1000.0f,
                                       2000.0f,
                                       ContractStatus::Open,
                                       false,
                                   });

  flecs::entity acceptedContract = world.entity("AcceptedContract")
                                        .set<Contract>({
                                            "TestClient",
                                            "TestDesc",
                                            1000.0f,
                                            2000.0f,
                                            ContractStatus::Accepted,
                                            false,
                                        });

  flecs::entity closedContract = world.entity("ClosedContract")
                                      .set<Contract>({
                                          "TestClient",
                                          "TestDesc",
                                          1000.0f,
                                          2000.0f,
                                          ContractStatus::Closed,
                                          false,
                                      });

  SECTION("All filter matches all statuses") {
    ContractsWindow state{ContractFilterStatus::All, true};
    REQUIRE(contractMatchesFilter(openContract, state));
    REQUIRE(contractMatchesFilter(acceptedContract, state));
    REQUIRE(contractMatchesFilter(closedContract, state));
  }

  SECTION("Open filter matches only Open") {
    ContractsWindow state{ContractFilterStatus::Open, true};
    REQUIRE(contractMatchesFilter(openContract, state));
    REQUIRE(!contractMatchesFilter(acceptedContract, state));
    REQUIRE(!contractMatchesFilter(closedContract, state));
  }

  SECTION("Accepted filter matches only Accepted") {
    ContractsWindow state{ContractFilterStatus::Accepted, true};
    REQUIRE(!contractMatchesFilter(openContract, state));
    REQUIRE(contractMatchesFilter(acceptedContract, state));
    REQUIRE(!contractMatchesFilter(closedContract, state));
  }

  SECTION("Closed filter matches only Closed") {
    ContractsWindow state{ContractFilterStatus::Closed, true};
    REQUIRE(!contractMatchesFilter(openContract, state));
    REQUIRE(!contractMatchesFilter(acceptedContract, state));
    REQUIRE(contractMatchesFilter(closedContract, state));
  }

  SECTION("showCompleted=false hides closed contracts") {
    ContractsWindow state{ContractFilterStatus::All, false};
    REQUIRE(contractMatchesFilter(openContract, state));
    REQUIRE(contractMatchesFilter(acceptedContract, state));
    REQUIRE(!contractMatchesFilter(closedContract, state));
  }
}

TEST_CASE("setupLaunchForContract creates necessary entities", "[contracts_window]") {
  flecs::world world;
  world.import<BaseModule>();
  world.import<SimulationModule>();
  world.import<RocketLaunchModule>();

  flecs::entity contract = world.entity("TestContract")
                               .set<Contract>({
                                   "TestClient",
                                   "TestDesc",
                                   1000.0f,
                                   2000.0f,
                                   ContractStatus::Accepted,
                                   false,
                               });

  flecs::entity planE = setupLaunchForContract(contract);

  // Verify plan was created
  REQUIRE(planE.is_valid());
  REQUIRE(planE.has<LaunchPlan>());

  // Verify payload was created and linked
  flecs::entity payloadE = planE.target<LaunchingWith>();
  REQUIRE(payloadE.is_valid());
  REQUIRE(payloadE.has<Payload>());

  // Verify relationships were set up
  REQUIRE(contract.target<ContractPayload>() == payloadE);
}
