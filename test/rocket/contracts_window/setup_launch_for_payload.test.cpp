#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/rocket/contracts_window.h"
#include "modules/rocket/launch_actions.h"
#include "modules/rocket/launch_window.h"
#include "modules/rocket/rocket_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

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
    registerWindow("Mission Plan", drawLaunchWindow, world)
        .set<LaunchWindow>({});

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
