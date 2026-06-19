#include "../helpers/setup_helpers.h"
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

      THEN("a building prefab is created under Prefabs::Buildings") {
        auto prefab = world.lookup("Prefabs::Buildings::Factory");
        REQUIRE(prefab.is_valid());
        CHECK(prefab.has<Building>());
      }

      THEN("each facility is created as a child with its type component") {
        auto manufacturing =
            world.lookup("Prefabs::Buildings::Factory::Line 1");
        REQUIRE(manufacturing.is_valid());
        CHECK(manufacturing.has<Manufacturing>());

        auto office = world.lookup("Prefabs::Buildings::Factory::Office 1");
        REQUIRE(office.is_valid());
        CHECK(office.has<Office>());
      }
    }
  }

  GIVEN("a building definition whose sprite clips a known texture") {
    // clip_sprite_from_texture looks the texture up by name under Textures::.
    auto textures = world.entity("Textures");
    auto texture = world.entity("Buildings").child_of(textures);

    BuildingDef hall;
    hall.id = "storage_hall";
    hall.name = "Storage Hall";
    hall.texture = "Buildings";
    hall.rect = {.x = 0, .y = 64, .width = 32, .height = 32};

    WHEN("the definition is applied") {
      applyBuildingData(world, {hall});

      THEN("the prefab's sprite carries the clip rect and texture reference") {
        auto prefab = world.lookup("Prefabs::Buildings::Storage Hall");
        REQUIRE(prefab.is_valid());
        REQUIRE(prefab.has<Sprite>());
        const auto &sprite = prefab.get<Sprite>();
        CHECK(sprite.x == 0);
        CHECK(sprite.y == 64);
        CHECK(sprite.width == 32);
        CHECK(sprite.height == 32);
        CHECK(sprite.texture == texture);
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
