#include "modules/site/build_mode.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Build mode tracks the prefab being placed", "[build_mode][site]") {
  flecs::world world;
  world.import <SiteModule>();
  auto factory = world.prefab("factory");
  auto office = world.prefab("office");

  GIVEN("the player is not building") {
    THEN("there is no build mode and no prefab") {
      CHECK_FALSE(world.has<BuildMode>());
      CHECK_FALSE(buildModePrefab(world).is_valid());
    }

    WHEN("a prefab is selected") {
      enterBuildMode(world, factory);

      THEN("build mode is active for that prefab, placing a building") {
        REQUIRE(world.has<BuildMode>());
        CHECK(buildModePrefab(world) == factory);
        CHECK(world.get<BuildMode>().kind == PlacementKind::Building);
        CHECK_FALSE(world.get<BuildMode>().dragging);
      }
    }

    WHEN("an invalid prefab is selected") {
      enterBuildMode(world, flecs::entity{});

      THEN("build mode stays off") { CHECK_FALSE(world.has<BuildMode>()); }
    }
  }

  GIVEN("the player is building one prefab") {
    enterBuildMode(world, factory);

    WHEN("another prefab is selected") {
      enterBuildMode(world, office);

      THEN("it replaces the first") {
        CHECK(buildModePrefab(world) == office);
        CHECK(world.entity<BuildMode>().target<BuildModePrefab>(1) ==
              flecs::entity{});
      }
    }

    WHEN("an in-progress drag is replaced by a new selection") {
      world.get_mut<BuildMode>().dragging = true;
      enterBuildMode(world, office);

      THEN("the drag is dropped") {
        CHECK_FALSE(world.get<BuildMode>().dragging);
      }
    }

    WHEN("build mode is left") {
      exitBuildMode(world);

      THEN("there is no build mode and no prefab") {
        CHECK_FALSE(world.has<BuildMode>());
        CHECK_FALSE(buildModePrefab(world).is_valid());
        CHECK_FALSE(
            world.entity<BuildMode>().has<BuildModePrefab>(flecs::Wildcard));
      }
    }

    WHEN("the selected prefab is deleted") {
      factory.destruct();

      THEN("no prefab is selected") {
        CHECK_FALSE(buildModePrefab(world).is_valid());
      }
    }
  }
}
