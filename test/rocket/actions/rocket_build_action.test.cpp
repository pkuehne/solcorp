#include "modules/rocket/actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("RocketBuildAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("An invalid prefab") {
    auto line = world.entity();
    RocketBuildAction action{PrefabEntity{flecs::entity::null()},
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
    RocketBuildAction action{PrefabEntity{prefab},
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
    RocketBuildAction action{PrefabEntity{prefab}, LineEntity{line}, 100};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Not enough funds to build this rocket");
      }
    }
  }

  GIVEN("A valid prefab, line, and sufficient balance") {
    auto prefab = world.prefab().add<Rocket>();
    auto line = world.entity();
    world.get_mut<Company>().balance = 500;
    RocketBuildAction action{PrefabEntity{prefab}, LineEntity{line}, 200};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It passes") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

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
        CHECK(created.has<RocketCurrentState>(
            world.lookup("States::Rocket::UnderConstruction")));
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
