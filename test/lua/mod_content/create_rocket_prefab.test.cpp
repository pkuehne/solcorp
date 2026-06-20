#include "../helpers/setup_helpers.h"
#include "modules/lua/mod_content.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_rocket_prefab", "[mod_content][lua]") {
  flecs::world world;
  run_rocket_prefab_setup(world);

  GIVEN("a prefab name") {
    WHEN("create_rocket_prefab is called") {
      auto prefab = create_rocket_prefab(world, "Falcon9");
      THEN("a valid prefab entity is returned") { REQUIRE(prefab.is_valid()); }
      THEN("it is a child of Prefabs::Rockets") {
        CHECK(prefab.parent() == world.lookup("Prefabs::Rockets"));
      }
    }

    WHEN("create_rocket_prefab is called twice with the same name") {
      auto first = create_rocket_prefab(world, "Falcon9");
      auto second = create_rocket_prefab(world, "Falcon9");
      THEN("the existing prefab is reused, not duplicated") {
        CHECK(first == second);

        int count = 0;
        world.lookup("Prefabs::Rockets").children([&](flecs::entity child) {
          if (child.name() == "Falcon9") {
            ++count;
          }
        });
        CHECK(count == 1);
      }
    }
  }
}
