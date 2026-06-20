#include "modules/lua/helpers.h"
#include "modules/lua/mod_content.h"
#include "setup_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_building", "[helpers][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);
  create_building_prefab(world, "Warehouse");
  auto site = world.entity("SiteA");

  GIVEN("a valid prefab name and site") {
    WHEN("create_building is called") {
      auto building = create_building(world, "WH1", "Warehouse", 3, 4, site);
      THEN("a valid entity is returned") { REQUIRE(building.is_valid()); }
      THEN("it is a child of the site") { CHECK(building.parent() == site); }
    }
  }

  GIVEN("a prefab name that does not exist") {
    WHEN("create_building is called") {
      auto building =
          create_building(world, "Ghost", "NoSuchPrefab", 0, 0, site);
      THEN("an invalid entity is returned") { CHECK(!building.is_valid()); }
    }
  }
}
