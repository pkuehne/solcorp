#include "modules/engine/render.h"
#include "modules/rocket/actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("ScheduleLaunchAction Validation", "[validation][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An empty plan") {
    ScheduleLaunchAction launch({});

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It is invalid") {
        CHECK(!result.ok);
        CHECK(result.message == "No rocket selected");
      }
    }
  }

  GIVEN("A rocket in Stored state") {
    auto rocket = world.entity().add<Rocket>();
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It still requires a launchpad") {
        CHECK(!result.ok);
        CHECK(result.message == "No launchpad selected");
      }
    }
  }

  GIVEN("A rocket in Assigned state") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }

  GIVEN("A rocket under construction") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }

  GIVEN("A rocket already assigned to another plan") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    rocket.add<LaunchingOn>(world.entity());
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }

  GIVEN("A missing launchpad") {
    auto rocket = world.entity().add<Rocket>();
    ScheduleLaunchAction launch;
    launch.rocket = rocket;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("Report the launchpad is missing") {
        CHECK(!result.ok);
        CHECK(result.message == "No launchpad selected");
      }
    }
  }

  GIVEN("A new plan and an existing plan with the same name") {
    world.entity("Test Plan").set<LaunchPlan>({});

    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("Report the name clash") {
        CHECK(!result.ok);
        CHECK(result.message ==
              "A Launch Plan named 'Test Plan' already exists");
      }
    }
  }

  GIVEN("A valid plan") {
    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);

      THEN("It succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }
}

SCENARIO("EditLaunchAction Validation", "[validation][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("No plan to edit") {
    EditLaunchAction edit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("It reports no plan to edit") {
        CHECK(!result.ok);
        CHECK(result.message == "No plan to edit");
      }
    }
  }

  GIVEN("The same name as another plan") {
    world.entity("Other Plan").set<LaunchPlan>({});
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(rocket);
    edit.launchDay = 10;
    edit.name = "Other Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("It reports the name clash") {
        CHECK(!result.ok);
        CHECK(result.message ==
              "A Launch Plan named 'Other Plan' already exists");
      }
    }
  }

  GIVEN("The same name as the plan being edited") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(rocket);
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }

  GIVEN("The same rocket already on this plan (Assigned state)") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(rocket);
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }

  GIVEN("A different rocket that is already Assigned") {
    auto oldRocket = world.entity().add<Rocket>();
    oldRocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto newRocket = world.entity().add<Rocket>();
    newRocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    newRocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    EditLaunchAction edit;
    edit.plan = world.entity("Test Plan").set<LaunchPlan>({});
    edit.plan.add<LaunchingOn>(oldRocket);
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = newRocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Validated") {
      ValidationResult result = edit.validate(world);

      THEN("It reports the rocket is not unassigned") {
        CHECK(!result.ok);
        CHECK(result.message == "Selected rocket is not unassigned");
      }
    }
  }
}

SCENARIO("EditLaunchAction Execution", "[execution][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An existing plan") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    auto existing =
        world.entity("Test Plan").set<LaunchPlan>({.launch_date = 5});
    existing.add<LaunchingOn>(rocket);
    existing.add<LaunchingFrom>(launchpad);

    EditLaunchAction edit;
    edit.plan = existing;
    edit.launchDay = 20;
    edit.name = "Test Plan";
    edit.rocket = rocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Executed") {
      edit.execute(world);

      THEN("The old plan is replaced with the new one") {
        REQUIRE(edit.result.is_valid());
        CHECK(!existing.is_alive());
        CHECK(std::cmp_equal(edit.result.get<LaunchPlan>().launch_date, 20));
        CHECK(edit.result.target<LaunchingOn>() == rocket);
      }
      THEN("The rocket remains Assigned") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Assigned);
      }
    }
  }

  GIVEN("An existing plan with a rocket being swapped") {
    auto oldRocket = world.entity().add<Rocket>();
    oldRocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto newRocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    newRocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    auto existing = world.entity("Test Plan").set<LaunchPlan>({});
    existing.add<LaunchingOn>(oldRocket);

    EditLaunchAction edit;
    edit.plan = existing;
    edit.launchDay = 10;
    edit.name = "Test Plan";
    edit.rocket = newRocket;
    edit.launchpad = launchpad;
    edit.targetOrbit = orbit;

    WHEN("Executed") {
      edit.execute(world);

      THEN("The old rocket is returned to Stored") {
        CHECK(oldRocket.get<Rocket>().state == RocketStateId::Stored);
      }
      THEN("The new rocket is Assigned") {
        CHECK(newRocket.get<Rocket>().state == RocketStateId::Assigned);
      }
    }
  }
}

SCENARIO("CancelLaunchAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An invalid plan") {
    CancelLaunchAction cancel;

    WHEN("Validated") {
      ValidationResult result = cancel.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not valid");
      }
    }
  }

  GIVEN("A valid plan with an assigned rocket") {
    auto rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
    auto plan = world.entity("Test Plan").set<LaunchPlan>({});
    plan.add<LaunchingOn>(rocket);
    CancelLaunchAction cancel{plan};

    WHEN("Executed") {
      cancel.execute(world);

      THEN("The plan is destroyed") { CHECK(!plan.is_alive()); }
      THEN("The rocket is returned to Stored") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Stored);
      }
    }
  }
}

SCENARIO("ScheduleLaunchAction Execution", "[execution][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("A valid plan") {
    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.targetOrbit = orbit;

    WHEN("Executed") {
      launch.execute(world);

      THEN("A launch plan is created") {
        REQUIRE(launch.result.is_valid());
        CHECK(std::cmp_equal(launch.result.get<LaunchPlan>().launch_date,
                             launch.launchDay));
        CHECK(launch.result.get<LaunchPlan>().target_orbit == orbit);
        CHECK(launch.result.name().c_str() == launch.name);
        CHECK(launch.result.target<LaunchingOn>() == rocket);
        CHECK(launch.result.target<LaunchingFrom>() == launchpad);
      }
      THEN("The rocket state is set to Assigned") {
        CHECK(rocket.get<Rocket>().state == RocketStateId::Assigned);
      }
    }
  }
}

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

SCENARIO("BuildRocketAction Validation", "[validation][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An invalid prefab") {
    auto line = world.entity();
    BuildRocketAction action{PrefabEntity{flecs::entity::null()},
                             LineEntity{line}, 100};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket prefab is not valid");
      }
    }
  }

  GIVEN("An invalid manufacturing line") {
    auto prefab = world.entity().add<Rocket>();
    BuildRocketAction action{PrefabEntity{prefab},
                             LineEntity{flecs::entity::null()}, 100};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Manufacturing line is not valid");
      }
    }
  }

  GIVEN("Insufficient company balance") {
    auto prefab = world.entity().add<Rocket>();
    auto line = world.entity();
    world.get_mut<Company>().balance = 50;
    BuildRocketAction action{PrefabEntity{prefab}, LineEntity{line}, 100};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Not enough funds to build this rocket");
      }
    }
  }

  GIVEN("A valid prefab, line, and sufficient balance") {
    auto prefab = world.entity().add<Rocket>();
    auto line = world.entity();
    world.get_mut<Company>().balance = 500;
    BuildRocketAction action{PrefabEntity{prefab}, LineEntity{line}, 100};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }
}

SCENARIO("BuildRocketAction Execution", "[execution][action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("A valid prefab, line, and sufficient balance") {
    auto prefab = world.prefab().add<Rocket>();
    auto line = world.entity();
    world.get_mut<Company>().balance = 500;
    BuildRocketAction action{PrefabEntity{prefab}, LineEntity{line}, 200};

    WHEN("Executed") {
      action.execute(world);

      THEN("A rocket is created as a child of the manufacturing line") {
        int count = 0;
        flecs::entity created = flecs::entity::null();
        line.children([&](flecs::entity child) {
          count++;
          created = child;
        });
        CHECK(count == 1);
        REQUIRE(created.is_valid());
        CHECK(created.get<Rocket>().state == RocketStateId::UnderConstruction);
        CHECK(created.has<EffortRequired>());
        CHECK(created.get<EffortRequired>().remaining == 300);
        CHECK(created.get<EffortRequired>().total == 300);
      }
      THEN("The cost is deducted from the company balance") {
        CHECK(world.get<Company>().balance == 300);
      }
    }
  }
}

SCENARIO("RocketMoveAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <MainModule>();
  world.import <RocketModule>();

  GIVEN("An invalid Rocket entity") {
    flecs::entity rocket = flecs::entity::null();
    flecs::entity destination = world.entity();
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message == "Rocket is not valid");
      }
    }
  }

  GIVEN("An invalid Destination") {
    flecs::entity rocket = world.entity().add<Rocket>();
    flecs::entity destination = flecs::entity::null();
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message == "Destination is not valid");
      }
    }
  }

  GIVEN("Destination is the same as current parent") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>().child_of(destination);
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message ==
              "Rocket is already stored in the selected destination");
      }
    }
  }

  GIVEN("A rocket under construction") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 3};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        CHECK(result.message == "Rocket is under construction");
      }
    }
  }

  GIVEN("A valid rocket and destination") {
    flecs::entity source = world.entity();
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>().child_of(source);
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});
    RocketMoveAction move = RocketMoveAction{RocketEntity{rocket},
                                             DestinationEntity{destination}, 5};

    WHEN("Validated") {
      ValidationResult result = move.validate(world);

      THEN("The validation succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      move.execute(world);

      THEN("The rocket remains in place until move completion") {
        CHECK(rocket.parent() == source);
      }
      THEN("The move target and duration are recorded") {
        CHECK(rocket.target<RocketTargetParent>() == destination);
        REQUIRE(rocket.has<RocketTargetState>());
        CHECK(rocket.get<RocketTargetState>().target == RocketStateId::Moving);
        REQUIRE(rocket.has<DurationRequired>());
        CHECK(rocket.get<DurationRequired>().remaining == 5);
        CHECK(rocket.get<DurationRequired>().total == 5);
      }
    }
  }
}

SCENARIO("RocketMoveCompleteAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <MainModule>();
  world.import <RocketModule>();

  GIVEN("An invalid rocket entity") {
    RocketMoveCompleteAction complete{flecs::entity::null()};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is not valid");
      }
    }
  }

  GIVEN("A rocket that is not currently moving") {
    auto rocket = world.entity().add<Rocket>();
    rocket.set<RocketTargetState>({.target = RocketStateId::Stored});
    RocketMoveCompleteAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is not currently moving");
      }
    }
  }

  GIVEN("A moving rocket without a target parent") {
    auto rocket = world.entity().add<Rocket>();
    rocket.set<RocketTargetState>({.target = RocketStateId::Moving});
    RocketMoveCompleteAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket does not have a valid target");
      }
    }
  }

  GIVEN("A moving rocket with time remaining") {
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>();
    rocket.set<RocketTargetState>({.target = RocketStateId::Moving});
    rocket.add<RocketTargetParent>(destination);
    rocket.set<DurationRequired>({.remaining = 3, .total = 5});
    RocketMoveCompleteAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket has not yet moved to the new location");
      }
    }
  }

  GIVEN("A moving rocket with valid target parent and duration") {
    auto source = world.entity();
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>().child_of(source);
    rocket.set<RocketTargetState>({.target = RocketStateId::Moving});
    rocket.add<RocketTargetParent>(destination);
    rocket.set<DurationRequired>({.remaining = 0, .total = 5});
    RocketMoveCompleteAction complete{rocket};

    WHEN("Validated") {
      ValidationResult result = complete.validate(world);

      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      complete.execute(world);

      THEN("The rocket is reparented and move markers are cleared") {
        CHECK(rocket.parent() == destination);
        CHECK(rocket.get<RocketTargetState>().target == RocketStateId::Stored);
        CHECK(!rocket.has<RocketTargetParent>());
        CHECK(!rocket.has<DurationRequired>());
      }
    }
  }
}

SCENARIO("RocketMoveCompleteAction Block", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <MainModule>();
  world.import <RocketModule>();

  GIVEN("A rocket that can complete move") {
    auto destination = world.entity();
    auto rocket = world.entity().add<Rocket>();
    rocket.set<RocketTargetState>({.target = RocketStateId::Moving});
    rocket.add<RocketTargetParent>(destination);

    WHEN("block is called") {
      RocketMoveCompleteAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is not set") {
        CHECK(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }

  GIVEN("A rocket missing move target") {
    auto rocket = world.entity().add<Rocket>();
    rocket.set<RocketTargetState>({.target = RocketStateId::Moving});

    WHEN("block is called") {
      RocketMoveCompleteAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        CHECK(rocket.get<RocketStateTransitionBlocked>().reason ==
              "Rocket does not have a valid target");
      }
    }
  }
}

SCENARIO("RocketCompleteBuildAction Block", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <MainModule>();
  world.import <RocketModule>();

  GIVEN("A rocket under construction with no effort remaining") {
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});

    WHEN("block is called") {
      RocketCompleteBuildAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is not set") {
        REQUIRE(!rocket.has<RocketStateTransitionBlocked>());
      }
    }
  }

  GIVEN("A rocket that is not under construction") {
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.set<EffortRequired>({.remaining = 0, .total = 300});

    WHEN("block is called") {
      RocketCompleteBuildAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        REQUIRE(rocket.get<RocketStateTransitionBlocked>().reason ==
                "Rocket is not under construction");
      }
    }
  }

  GIVEN("A rocket under construction with effort still remaining") {
    flecs::entity rocket = world.entity().add<Rocket>();
    rocket.get_mut<Rocket>().state = RocketStateId::UnderConstruction;
    rocket.set<EffortRequired>({.remaining = 50, .total = 300});

    WHEN("block is called") {
      RocketCompleteBuildAction{rocket}.block(world);

      THEN("RocketStateTransitionBlocked is set with the reason") {
        REQUIRE(rocket.has<RocketStateTransitionBlocked>());
        REQUIRE(rocket.get<RocketStateTransitionBlocked>().reason ==
                "Rocket construction is not yet complete");
      }
    }
  }
}
