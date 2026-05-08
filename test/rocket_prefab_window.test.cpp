#include "modules/site/rocket_prefab_window.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("collectRocketPrefabs returns all rocket prefabs", "[site][ui]") {
  flecs::world world;
  world.component<Rocket>();

  auto prefabs = world.entity("Prefabs");
  auto rockets = world.entity("Rockets").child_of(prefabs);

  GIVEN("two rocket prefabs and one non-rocket prefab") {
    world.prefab("Falcon 1").child_of(rockets).set<Rocket>({});
    world.prefab("Falcon 9").child_of(rockets).set<Rocket>({});
    world.prefab("Not a Rocket").child_of(rockets);

    WHEN("collecting rocket prefab options") {
      auto options = collectRocketPrefabs(world);

      THEN("exactly the two rocket prefabs are returned") {
        REQUIRE(options.size() == 2);
      }
    }
  }
}

SCENARIO("computeRocketPrefabBuildCost applies modifiers", "[site][ui]") {
  flecs::world world;
  world.component<Rocket>();
  world.component<HasEffect>();
  world.component<Modifier>();

  auto prefabs = world.entity("Prefabs");
  auto rockets = world.entity("Rockets").child_of(prefabs);

  GIVEN("a rocket prefab with base cost and a matching additive modifier") {
    auto prefab = world.prefab("Falcon 1").child_of(rockets).set<Rocket>({});

    auto &rocket = prefab.get_mut<Rocket>();
    rocket.cost.setBase(1000);

    auto effect = world.entity("Cost Reduction Program");
    world.entity().child_of(effect).set<Modifier>(
        {.target_stat = "cost", .additive = 250.0, .multiplicative = 1.0});
    prefab.add<HasEffect>(effect);

    WHEN("computing rocket prefab build cost") {
      auto cost = computeRocketPrefabBuildCost(prefab);

      THEN("the final cost includes applied modifiers") {
        REQUIRE(cost == 1250);
      }
    }
  }
}

SCENARIO("canAffordRocketPrefab uses modifier-aware cost", "[site][ui]") {
  flecs::world world;
  world.component<Rocket>();
  world.component<HasEffect>();
  world.component<Modifier>();

  auto prefabs = world.entity("Prefabs");
  auto rockets = world.entity("Rockets").child_of(prefabs);

  GIVEN("a rocket prefab with a modifier-adjusted build cost of 1250") {
    auto prefab = world.prefab("Falcon 1").child_of(rockets).set<Rocket>({});

    auto &rocket = prefab.get_mut<Rocket>();
    rocket.cost.setBase(1000);

    auto effect = world.entity("Cost Reduction Program");
    world.entity().child_of(effect).set<Modifier>(
        {.target_stat = "cost", .additive = 250.0, .multiplicative = 1.0});
    prefab.add<HasEffect>(effect);

    WHEN("company balance is below required cost") {
      Company company;
      company.balance = 1249;

      THEN("the prefab is not affordable") {
        REQUIRE_FALSE(canAffordRocketPrefab(company, prefab));
      }
    }

    WHEN("company balance matches required cost") {
      Company company;
      company.balance = 1250;

      THEN("the prefab is affordable") {
        REQUIRE(canAffordRocketPrefab(company, prefab));
      }
    }
  }
}
