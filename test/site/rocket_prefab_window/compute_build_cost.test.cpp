#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/rocket_prefab_window.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

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
