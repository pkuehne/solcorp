#include "modules/base/base.h"
#include "modules/rocket/rocket_module.h"
#include "modules/window/contracts_window.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Displaying contracts in the ContractListWindow",
         "[contracts_window]") {
  flecs::world world;
  world.import <BaseModule>();
  world.import <RocketModule>();

  world.entity("OpenContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .failed = false})
      .add<ContractCurrentState>(world.lookup("States::Contract::Open"));
  world.entity("AcceptedContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .failed = false})
      .add<ContractCurrentState>(world.lookup("States::Contract::Accepted"));
  world.entity("ClosedContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .failed = false})
      .add<ContractCurrentState>(world.lookup("States::Contract::Closed"));
  world.entity("FailedContract")
      .set<Contract>({.client = "TestClient",
                      .description = "TestDesc",
                      .upfront_payment = 1000u,
                      .completion_payment = 2000u,
                      .failed = true})
      .add<ContractCurrentState>(world.lookup("States::Contract::Closed"));

  WHEN("ContractListWindow with All filter and hideTerminal=false") {
    ContractListWindow state{.stateFilter = flecs::entity::null(),
                             .hideTerminal = false};
    THEN("All contracts match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractListWindow with Open filter") {
    ContractListWindow state{.stateFilter =
                                 world.lookup("States::Contract::Open"),
                             .hideTerminal = false};
    THEN("Only Open contracts match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractListWindow with hideTerminal=true") {
    ContractListWindow state{.stateFilter = flecs::entity::null(),
                             .hideTerminal = true};
    THEN("Closed contracts do not match the filter") {
      REQUIRE(contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractListWindow with Closed filter") {
    ContractListWindow state{.stateFilter =
                                 world.lookup("States::Contract::Closed"),
                             .hideTerminal = false};
    THEN("Only Closed contracts match the filter") {
      REQUIRE(!contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }

  WHEN("ContractListWindow with Accepted filter") {
    ContractListWindow state{.stateFilter =
                                 world.lookup("States::Contract::Accepted"),
                             .hideTerminal = false};
    THEN("Only Accepted contracts match the filter") {
      REQUIRE(!contractMatchesFilter(world.entity("OpenContract"), state));
      REQUIRE(contractMatchesFilter(world.entity("AcceptedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("ClosedContract"), state));
      REQUIRE(!contractMatchesFilter(world.entity("FailedContract"), state));
    }
  }
}
