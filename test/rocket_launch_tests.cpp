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
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
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
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
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
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan Bravo";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
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
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
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
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.rocket = rocket;

    WHEN("Executed") {
      launch.execute(world);
      THEN("A launch plan is created") {
        REQUIRE(launch.result.is_valid());
        CHECK(launch.result.get<LaunchPlan>().launch_date ==
              static_cast<u_int>(launch.launchDay));
        CHECK(launch.result.name().c_str() == launch.name);
        CHECK(launch.result.target<LaunchingOn>() == rocket);
        CHECK(launch.result.target<LaunchingFrom>() == launchpad);
      }
    }
  }

  GIVEN("The same plan executed twice") {
    auto rocket = world.entity().add<Rocket>();
    auto launchpad = world.entity().add<Launchpad>();
    ScheduleLaunchAction launch;
    launch.launchDay = 10;
    launch.name = "Test Plan";
    launch.rocket = rocket;
    launch.launchpad = launchpad;
    launch.rocket = rocket;
    launch.current = world.entity("Test Plan").set<LaunchPlan>({});

    WHEN("Executed") {
      launch.execute(world);
      THEN("A launch plan is created") {
        REQUIRE(launch.result.is_valid());
        REQUIRE(launch.current.is_alive() == false);
        CHECK(launch.result.get<LaunchPlan>().launch_date ==
              static_cast<u_int>(launch.launchDay));
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

  auto site = world.entity().add<Site>().set<CurrentSite>({});
  auto launchpad = world.entity("Main Pad")
                       .is_a<Launchpad>()
                       .child_of(site)
                       .set<SiteLocation>({0, 0})
                       .set<Transform>({})
                       .set<Sprite>({});
  auto rocket = world.entity("Falcon 9").add<Rocket>();

  GIVEN("A launch plan due today") {
    u_int today = world.get<Game>().day;
    auto planE = world.entity("Test Plan")
                     .set<LaunchPlan>({today})
                     .add<LaunchingOn>(rocket)
                     .add<LaunchingFrom>(launchpad);
    REQUIRE(planE.is_valid());
    REQUIRE(planE.get<LaunchPlan>().launch_date == today);
    REQUIRE(rocket.is_valid());

    WHEN("The launch system runs") {
      systemLaunchRocket(planE, planE.get_mut<LaunchPlan>());
      THEN("The rocket is removed") {
        CHECK(!rocket.is_alive());
        CHECK(!planE.is_alive());
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
