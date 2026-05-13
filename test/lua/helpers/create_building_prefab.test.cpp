#include "modules/lua/helpers.h"
#include "setup_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_building_prefab", "[helpers][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);

  GIVEN("a prefab name") {
    WHEN("create_building_prefab is called") {
      auto prefab = create_building_prefab(world, "ControlCenter");
      THEN("a valid prefab entity is returned") { REQUIRE(prefab.is_valid()); }
      THEN("it is a child of Prefabs::Buildings") {
        CHECK(prefab.parent() == world.lookup("Prefabs::Buildings"));
      }
    }
  }
}
