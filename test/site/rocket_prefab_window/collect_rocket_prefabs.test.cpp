#include "modules/rocket/rocket_module.h"
#include "modules/site/rocket_prefab_window.h"
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
