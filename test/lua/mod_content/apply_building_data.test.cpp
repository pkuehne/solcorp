#include "../helpers/setup_helpers.h"
#include "modules/base/base.h"
#include "modules/lua/mod_content.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("applyBuildingData creates building prefabs with facilities",
         "[mod_content][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);

  GIVEN("a building definition with two typed facilities") {
    BuildingDef factory;
    factory.id = "factory";
    factory.name = "Factory";
    factory.facilities = {
        {.name = "Line 1", .type = "Manufacturing"},
        {.name = "Office 1", .type = "Office"},
    };

    WHEN("the definition is applied") {
      applyBuildingData(world, {factory});

      THEN(
          "a building prefab is created under Prefabs::Buildings keyed by id") {
        auto prefab = world.lookup("Prefabs::Buildings::factory");
        REQUIRE(prefab.is_valid());
        CHECK(prefab.has<Building>());
      }

      THEN("the prefab carries the display name as a Label") {
        auto prefab = world.lookup("Prefabs::Buildings::factory");
        REQUIRE(prefab.is_valid());
        REQUIRE(prefab.has<Label>());
        CHECK(prefab.get<Label>().label == "Factory");
      }

      THEN("each facility is created as a child with its type component") {
        auto manufacturing =
            world.lookup("Prefabs::Buildings::factory::Line 1");
        REQUIRE(manufacturing.is_valid());
        CHECK(manufacturing.has<Manufacturing>());

        auto office = world.lookup("Prefabs::Buildings::factory::Office 1");
        REQUIRE(office.is_valid());
        CHECK(office.has<Office>());
      }
    }
  }

  GIVEN("the same building definition applied twice (a reload)") {
    BuildingDef factory;
    factory.id = "factory";
    factory.name = "Factory";
    factory.facilities = {
        {.name = "Line 1", .type = "Manufacturing"},
    };

    WHEN("the definition is applied a second time") {
      applyBuildingData(world, {factory});
      applyBuildingData(world, {factory});

      THEN("the prefab and its facility are reused, not duplicated") {
        auto prefab = world.lookup("Prefabs::Buildings::factory");
        REQUIRE(prefab.is_valid());

        int building_count = 0;
        world.lookup("Prefabs::Buildings").children([&](flecs::entity child) {
          if (child.name() == "factory") {
            ++building_count;
          }
        });
        CHECK(building_count == 1);

        int line_count = 0;
        prefab.children([&](flecs::entity child) {
          if (child.name() == "Line 1") {
            ++line_count;
          }
        });
        CHECK(line_count == 1);
        CHECK(world.lookup("Prefabs::Buildings::factory::Line 1")
                  .has<Manufacturing>());
      }
    }
  }

  GIVEN("a building definition whose tile is in a known tileset") {
    auto tilesets = world.entity("Tilesets");
    auto tileset = world.entity("buildings").child_of(tilesets);

    BuildingDef hall;
    hall.id = "storage_hall";
    hall.name = "Storage Hall";
    hall.tileset = "buildings";
    hall.tile_index = 2;

    WHEN("the definition is applied") {
      applyBuildingData(world, {hall});

      THEN("the prefab is drawn with that tile from that tileset") {
        auto prefab = world.lookup("Prefabs::Buildings::storage_hall");
        REQUIRE(prefab.is_valid());
        REQUIRE(prefab.has<Tile>(tileset));
        CHECK(prefab.get<Tile>(tileset).index == 2);
      }

      THEN("the prefab has no sprite to draw underneath the tile") {
        auto prefab = world.lookup("Prefabs::Buildings::storage_hall");
        REQUIRE(prefab.is_valid());
        CHECK_FALSE(prefab.has<Sprite>());
      }
    }
  }

  GIVEN("a building definition whose tileset does not exist") {
    BuildingDef hall;
    hall.id = "storage_hall";
    hall.name = "Storage Hall";
    hall.tileset = "no_such_tileset";
    hall.tile_index = 2;

    WHEN("the definition is applied") {
      applyBuildingData(world, {hall});

      THEN("the prefab is still created, without a tile") {
        auto prefab = world.lookup("Prefabs::Buildings::storage_hall");
        REQUIRE(prefab.is_valid());
        CHECK_FALSE(prefab.target<Tile>().is_valid());
      }
    }
  }
}

SCENARIO("addFacilityComponent maps facility type strings to components",
         "[mod_content][lua]") {
  flecs::world world;
  run_site_prefab_setup(world);

  GIVEN("a facility entity") {
    auto facility = world.entity("Facility");

    WHEN("a known type is applied") {
      bool ok = addFacilityComponent(facility, "Launchpad");

      THEN("it returns true and adds the matching component") {
        CHECK(ok);
        CHECK(facility.has<Launchpad>());
      }
    }

    WHEN("an unknown type is applied") {
      bool ok = addFacilityComponent(facility, "Teleporter");

      THEN("it returns false and adds nothing") {
        CHECK_FALSE(ok);
        CHECK_FALSE(facility.has<Launchpad>());
        CHECK_FALSE(facility.has<Office>());
        CHECK_FALSE(facility.has<Storage>());
        CHECK_FALSE(facility.has<Manufacturing>());
      }
    }
  }
}
