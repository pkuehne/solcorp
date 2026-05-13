#include "setup_helpers.h"
#include "modules/lua/helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_rocket_prefab", "[helpers][lua]") {
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
  }
}
