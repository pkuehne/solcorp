#include "../helpers/setup_helpers.h"
#include "modules/lua/mod_content.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("add_facility_to_building", "[helpers][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);
  auto building = world.entity("HQ");

  GIVEN("a valid building entity") {
    WHEN("add_facility_to_building is called") {
      auto facility =
          add_facility_to_building(world, building, "MissionControl");
      THEN("a valid entity is returned") { REQUIRE(facility.is_valid()); }
      THEN("it is a child of the building") {
        CHECK(facility.parent() == building);
      }
    }
  }
}
