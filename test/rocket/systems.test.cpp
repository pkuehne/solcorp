#include "modules/engine/render.h"
#include "modules/window/window_module.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("systemCreateRocketPrefabs", "[rocket][system]") {
  flecs::world world;
  world.import <RocketModule>();

  GIVEN("An empty world") {
    auto system = world.system("Create Rocket Prefabs")
                      .kind(flecs::OnStart)
                      .immediate()
                      .run(systemCreateRocketPrefabs);
    WHEN("The system is run") {
      system.run();
      THEN("The Prefabs::Rockets node is created") {
        auto node = world.lookup("Prefabs::Rockets");
        CHECK(node.is_valid());
      }
      THEN("The Prefabs::Core node contains a Rocket prefab") {
        auto node = world.lookup("Prefabs::Core::Rocket");
        CHECK(node.is_valid());
        CHECK(node.has<Rocket>());
      }
    }
  }
}

SCENARIO("systemLaunchRocket", "[rocket][system]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  auto site = world.entity().add<Site>().add<CurrentSite>();
  auto launchpad = world.entity("Main Pad")
                       .add<Launchpad>()
                       .child_of(site)
                       .set<SiteLocation>({.x = 0, .y = 0})
                       .set<Transform>({})
                       .set<Sprite>({});
  auto rocket = world.entity("Falcon 9").add<Rocket>();
  rocket.get_mut<Rocket>().failure_rate.setBase(0.0);

  GIVEN("A launch plan due today with multiple payloads and matching contract "
        "orbit") {
    uint32_t today = world.get<Game>().day;
    auto targetOrbit = world.entity("LEO");
    auto contractA = world.entity("Contract A1")
                         .set<Contract>({
                             .client = "Client",
                             .description = "Description",
                             .upfront_payment = 1000u,
                             .completion_payment = 2000u,
                             .status = ContractStatus::Accepted,
                             .failed = false,
                         });
    auto contractB = world.entity("Contract A2")
                         .set<Contract>({
                             .client = "Client",
                             .description = "Description",
                             .upfront_payment = 1000u,
                             .completion_payment = 2000u,
                             .status = ContractStatus::Accepted,
                             .failed = false,
                         });
    contractA.add<ContractTargetOrbit>(targetOrbit);
    contractB.add<ContractTargetOrbit>(targetOrbit);
    auto payloadA = world.entity("Payload A1").set<Payload>({1000});
    auto payloadB = world.entity("Payload A2").set<Payload>({2000});
    contractA.add<ContractPayload>(payloadA);
    contractB.add<ContractPayload>(payloadB);

    auto planE = world.entity("Test Plan")
                     .set<LaunchPlan>(
                         {.launch_date = today, .target_orbit = targetOrbit})
                     .add<LaunchingOn>(rocket)
                     .add<LaunchingFrom>(launchpad)
                     .add<LaunchingWith>(payloadA)
                     .add<LaunchingWith>(payloadB);
    REQUIRE(planE.is_valid());
    REQUIRE(planE.get<LaunchPlan>().launch_date == today);
    REQUIRE(rocket.is_valid());
    REQUIRE(payloadA.is_valid());
    REQUIRE(payloadB.is_valid());
    REQUIRE(contractA.is_valid());
    REQUIRE(contractB.is_valid());

    WHEN("The launch system runs") {
      systemLaunchRocket(planE, planE.get_mut<LaunchPlan>());
      THEN("The rocket and all payloads are removed and contracts are closed") {
        CHECK(!rocket.is_alive());
        CHECK(!payloadA.is_alive());
        CHECK(!payloadB.is_alive());
        CHECK(!planE.is_alive());
        CHECK(contractA.is_alive());
        CHECK(contractB.is_alive());
        CHECK(contractA.get<Contract>().status == ContractStatus::Closed);
        CHECK(contractB.get<Contract>().status == ContractStatus::Closed);
        CHECK(contractA.get<Contract>().failed == false);
        CHECK(contractB.get<Contract>().failed == false);
      }
      THEN("The completion payments are added to the company balance") {
        CHECK(world.get<Company>().balance == 4000);
      }
    }
  }

  GIVEN(
      "A launch plan due today with multiple payloads and mismatched contract "
      "orbit") {
    uint32_t today = world.get<Game>().day;
    auto launchedOrbit = world.entity("LEO");
    auto contractOrbit = world.entity("GTO");
    auto contractA = world.entity("Contract B1")
                         .set<Contract>({
                             .client = "Client",
                             .description = "Description",
                             .upfront_payment = 1000u,
                             .completion_payment = 2000u,
                             .status = ContractStatus::Accepted,
                             .failed = false,
                         });
    auto contractB = world.entity("Contract B2")
                         .set<Contract>({
                             .client = "Client",
                             .description = "Description",
                             .upfront_payment = 1000u,
                             .completion_payment = 2000u,
                             .status = ContractStatus::Accepted,
                             .failed = false,
                         });
    contractA.add<ContractTargetOrbit>(contractOrbit);
    contractB.add<ContractTargetOrbit>(contractOrbit);
    auto payloadA = world.entity("Payload B1").set<Payload>({1000});
    auto payloadB = world.entity("Payload B2").set<Payload>({2000});
    contractA.add<ContractPayload>(payloadA);
    contractB.add<ContractPayload>(payloadB);

    auto planE = world.entity("Test Plan 2")
                     .set<LaunchPlan>(
                         {.launch_date = today, .target_orbit = launchedOrbit})
                     .add<LaunchingOn>(rocket)
                     .add<LaunchingFrom>(launchpad)
                     .add<LaunchingWith>(payloadA)
                     .add<LaunchingWith>(payloadB);
    REQUIRE(planE.is_valid());
    REQUIRE(planE.get<LaunchPlan>().launch_date == today);
    REQUIRE(rocket.is_valid());
    REQUIRE(payloadA.is_valid());
    REQUIRE(payloadB.is_valid());
    REQUIRE(contractA.is_valid());
    REQUIRE(contractB.is_valid());

    WHEN("The launch system runs") {
      systemLaunchRocket(planE, planE.get_mut<LaunchPlan>());
      THEN("All contracts are marked failed and closed") {
        CHECK(!rocket.is_alive());
        CHECK(!payloadA.is_alive());
        CHECK(!payloadB.is_alive());
        CHECK(!planE.is_alive());
        CHECK(contractA.is_alive());
        CHECK(contractB.is_alive());
        CHECK(contractA.get<Contract>().status == ContractStatus::Closed);
        CHECK(contractB.get<Contract>().status == ContractStatus::Closed);
        CHECK(contractA.get<Contract>().failed == true);
        CHECK(contractB.get<Contract>().failed == true);
      }
      THEN("No payment is added to the company balance") {
        CHECK(world.get<Company>().balance == 0);
      }
    }
  }

  GIVEN("A launch plan not due yet") {
    uint32_t today = world.get<Game>().day;
    auto planE = world.entity("Test Plan")
                     .set<LaunchPlan>({.launch_date = today + 1})
                     .add<LaunchingOn>(rocket)
                     .add<LaunchingFrom>(launchpad);
    REQUIRE(planE.is_valid());
    REQUIRE(planE.get<LaunchPlan>().launch_date == today + 1);
    REQUIRE(rocket.is_valid());

    WHEN("The launch system runs") {
      systemLaunchRocket(planE, planE.get_mut<LaunchPlan>());
      THEN("The rocket remains") {
        CHECK(rocket.is_alive());
        CHECK(planE.is_alive());
      }
    }
  }
}

SCENARIO("systemRocketCompleteAction", "[system]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <WindowModule>();
  world.import <RocketModule>();

  GIVEN("A rocket under construction with effort still remaining") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    rocket.set<EffortRequired>({.remaining = 50, .total = 300});
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket);

      THEN("The rocket is blocked and remains under construction") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::UnderConstruction);
        CHECK(rocket.has<EffortRequired>());
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        CHECK(rocket.get<RocketStateTransitionBlocked>().reason ==
              "Rocket construction is not yet complete");
      }
    }
  }

  GIVEN("A rocket under construction with effort complete") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket);

      THEN("The rocket transitions to Stored and EffortRequired is removed") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
        CHECK(!rocket.has<EffortRequired>());
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }

  GIVEN("A rocket in Stored state that unexpectedly has EffortRequired") {
    auto rocket = world.entity().add<Rocket>(); // default state: Stored
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket);

      THEN("Nothing happens — the system ignores states other than "
           "UnderConstruction and Moving") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
        CHECK(rocket.has<EffortRequired>());
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }

  GIVEN("A moving rocket with duration still remaining") {
    auto source = world.entity();
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(source);
    rocket.get_mut<Rocket>().state = RocketStateId::Moving;
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});
    rocket.add<RocketTargetParent>(destination);
    rocket.set<DurationRequired>({.remaining = 3, .total = 5});

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket);

      THEN("The rocket is blocked and remains at its current location") {
        CHECK(rocket.parent() == source);
        CHECK(rocket.has<DurationRequired>());
        CHECK(rocket.get<DurationRequired>().remaining == 3);
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        CHECK(rocket.get<RocketStateTransitionBlocked>().reason ==
              "Rocket has not yet moved to the new location");
      }
    }
  }

  GIVEN("A moving rocket with duration complete") {
    auto source = world.entity();
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(source);
    rocket.get_mut<Rocket>().state = RocketStateId::Moving;
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});
    rocket.add<RocketTargetParent>(destination);
    rocket.set<DurationRequired>({.remaining = 0, .total = 5});

    WHEN("systemRocketCompleteAction is called") {
      systemRocketCompleteAction(rocket);

      THEN("The rocket is reparented to its destination and move markers are "
           "cleared") {
        CHECK(rocket.parent() == destination);
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
        CHECK(!rocket.has<RocketTargetState>());
        CHECK(!rocket.has<RocketTargetParent>());
        CHECK(!rocket.has<DurationRequired>());
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }
}
