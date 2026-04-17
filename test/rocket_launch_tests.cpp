#include "modules/engine/render.h"
#include "modules/rocket_launch/actions.h"
#include "modules/rocket_launch/rocket_launch.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("ScheduleLaunchAction Validation", "[validation][action]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

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

  GIVEN("A valid rocket") {
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

  GIVEN("A pre-allocated rocket") {
    auto rocket = world.entity().add<Rocket>();
    rocket.add<LaunchingOn>(world.entity());
    ScheduleLaunchAction launch;
    launch.rocket = rocket;
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("Report the rocket is already planned") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is already planned for a launch");
      }
    }
  }

  GIVEN("A missing launchpad") {
    auto rocket = world.entity().add<Rocket>();
    ScheduleLaunchAction launch;
    launch.rocket = rocket;
    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("Report the launchpad  is missing") {
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

  GIVEN("A new plan with an attached rocket") {
    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    world.entity("Other Test Plan")
        .set<LaunchPlan>({})
        .add<LaunchingOn>(rocket);

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("Report the pre-allocated rocket") {
        CHECK(!result.ok);
        CHECK(result.message == "Rocket is already planned for a launch");
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

  GIVEN("The current plan with the same name") {
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
    launch.current = world.entity("Test Plan").set<LaunchPlan>({});

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }

  GIVEN("The current plan with a different name") {
    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan Bravo";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.targetOrbit = orbit;
    launch.current = world.entity("Test Plan").set<LaunchPlan>({});

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }

  GIVEN("The current plan with an attached rocket") {
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
    launch.current = world.entity("Test Plan").set<LaunchPlan>({});
    launch.current.add<LaunchingOn>(rocket);

    WHEN("Validated") {
      ValidationResult result = launch.validate(world);
      THEN("The validation passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }
  }
}

SCENARIO("ScheduleLaunchAction Execution", "[execution][action]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

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
    launch.rocket = rocket;
    launch.targetOrbit = orbit;

    WHEN("Executed") {
      launch.execute(world);
      THEN("A launch plan is created") {
        REQUIRE(launch.result.is_valid());
        CHECK(launch.result.get<LaunchPlan>().launch_date ==
              static_cast<u_int>(launch.launchDay));
        CHECK(launch.result.get<LaunchPlan>().target_orbit == orbit);
        CHECK(launch.result.name().c_str() == launch.name);
        CHECK(launch.result.target<LaunchingOn>() == rocket);
        CHECK(launch.result.target<LaunchingFrom>() == launchpad);
      }
    }
  }

  GIVEN("The same plan executed twice") {
    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    auto orbit = world.entity("LEO");
    rocket.set<CanLiftTo>(orbit, {.max_mass = 1000});
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.rocket = rocket;
    launch.targetOrbit = orbit;
    launch.current = world.entity("Test Plan").set<LaunchPlan>({});

    WHEN("Executed") {
      launch.execute(world);
      THEN("A launch plan is created") {
        REQUIRE(launch.result.is_valid());
        REQUIRE(launch.current.is_alive() == false);
        CHECK(launch.result.get<LaunchPlan>().launch_date ==
              static_cast<u_int>(launch.launchDay));
        CHECK(launch.result.get<LaunchPlan>().target_orbit == orbit);
        CHECK(launch.result.name().c_str() == launch.name);
        CHECK(launch.result.target<LaunchingOn>() == rocket);
        CHECK(launch.result.target<LaunchingFrom>() == launchpad);
      }
    }
  }
}

SCENARIO("systemCreateRocketPrefabs", "[rocket_launch][system]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

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

SCENARIO("systemLaunchRocket", "[rocket_launch][system]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

  auto site = world.entity().add<Site>().add<CurrentSite>();
  auto launchpad = world.entity("Main Pad")
                       .is_a<Launchpad>()
                       .child_of(site)
                       .set<SiteLocation>({0, 0})
                       .set<Transform>({})
                       .set<Sprite>({});
  auto rocket = world.entity("Falcon 9").add<Rocket>();

  GIVEN("A launch plan due today with multiple payloads and matching contract "
        "orbit") {
    u_int today = world.get<Game>().day;
    auto targetOrbit = world.entity("LEO");
    auto contractA = world.entity("Contract A1")
                         .set<Contract>({
                             "Client",
                             "Description",
                             1000.0f,
                             2000.0f,
                             ContractStatus::Accepted,
                             false,
                         });
    auto contractB = world.entity("Contract A2")
                         .set<Contract>({
                             "Client",
                             "Description",
                             1000.0f,
                             2000.0f,
                             ContractStatus::Accepted,
                             false,
                         });
    contractA.add<ContractTargetOrbit>(targetOrbit);
    contractB.add<ContractTargetOrbit>(targetOrbit);
    auto payloadA = world.entity("Payload A1").set<Payload>({1000});
    auto payloadB = world.entity("Payload A2").set<Payload>({2000});
    contractA.add<ContractPayload>(payloadA);
    contractB.add<ContractPayload>(payloadB);

    auto planE = world.entity("Test Plan")
                     .set<LaunchPlan>({today, targetOrbit})
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
    }
  }

  GIVEN(
      "A launch plan due today with multiple payloads and mismatched contract "
      "orbit") {
    u_int today = world.get<Game>().day;
    auto launchedOrbit = world.entity("LEO");
    auto contractOrbit = world.entity("GTO");
    auto contractA = world.entity("Contract B1")
                         .set<Contract>({
                             "Client",
                             "Description",
                             1000.0f,
                             2000.0f,
                             ContractStatus::Accepted,
                             false,
                         });
    auto contractB = world.entity("Contract B2")
                         .set<Contract>({
                             "Client",
                             "Description",
                             1000.0f,
                             2000.0f,
                             ContractStatus::Accepted,
                             false,
                         });
    contractA.add<ContractTargetOrbit>(contractOrbit);
    contractB.add<ContractTargetOrbit>(contractOrbit);
    auto payloadA = world.entity("Payload B1").set<Payload>({1000});
    auto payloadB = world.entity("Payload B2").set<Payload>({2000});
    contractA.add<ContractPayload>(payloadA);
    contractB.add<ContractPayload>(payloadB);

    auto planE = world.entity("Test Plan 2")
                     .set<LaunchPlan>({today, launchedOrbit})
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
    }
  }

  GIVEN("A launch plan not due yet") {
    u_int today = world.get<Game>().day;
    auto planE = world.entity("Test Plan")
                     .set<LaunchPlan>({today + 1})
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

SCENARIO("MoveRocketAction Validation", "[validation][action]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

  GIVEN("An invalid Rocket entity") {
    flecs::entity rocket = flecs::entity::null();
    flecs::entity destination = world.entity();
    MoveRocketAction move = MoveRocketAction(rocket, destination);

    WHEN("Validate is called") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        //
      }
    }
  }

  GIVEN("An invalid Destination ") {
    flecs::entity rocket = world.entity().add<Rocket>();
    flecs::entity destination = flecs::entity::null();
    MoveRocketAction move = MoveRocketAction(rocket, destination);

    WHEN("Validate is called") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        //
      }
    }
  }

  GIVEN("Destination is the same as parent") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>().child_of(destination);
    MoveRocketAction move = MoveRocketAction(rocket, destination);

    WHEN("Validate is called") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        //
      }
    }
  }

  GIVEN("Rocket has Construction tag") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>().add<Construction>();
    MoveRocketAction move = MoveRocketAction(rocket, destination);

    WHEN("Validate is called") {
      ValidationResult result = move.validate(world);

      THEN("The validation fails") {
        CHECK(!result);
        //
      }
    }
  }

  GIVEN("Valid rocket and destination") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>();
    MoveRocketAction move = MoveRocketAction(rocket, destination);

    WHEN("Validate is called") {
      ValidationResult result = move.validate(world);

      THEN("The validation succeeds") {
        CHECK(result.ok);
        //
      }
    }
  }
}

SCENARIO("MoveRocketAction Execution", "[execution][action]") {
  flecs::world world;
  world.import <RocketLaunchModule>();

  GIVEN("A valid rocket and destination") {
    flecs::entity destination = world.entity();
    flecs::entity rocket = world.entity().add<Rocket>();
    MoveRocketAction move = MoveRocketAction(rocket, destination);

    WHEN("Move Rocket is attempted") {
      move.execute(world);

      THEN("The rocket is moved to the destination") {
        CHECK(rocket.parent() == destination);
        //
      }
    }
  }
}
