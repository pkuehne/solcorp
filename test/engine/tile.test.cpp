#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/engine/render.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Tiles are inherited from prefabs", "[tileset]") {
  flecs::world world;
  world.import <BaseModule>();
  registerEngineComponents(world);
  auto tileset = world.entity("buildings");

  GIVEN("a prefab with a tile from a tileset") {
    auto prefab = world.prefab("Warehouse").set<Tile>(tileset, {.index = 3});

    WHEN("an instance of the prefab is created") {
      auto instance = world.entity().is_a(prefab);

      THEN("the instance has the prefab's tile and tileset") {
        REQUIRE(instance.has<Tile>(tileset));
        CHECK(instance.get<Tile>(tileset).index == 3);
      }
    }

    WHEN("tiles in any tileset are queried") {
      auto instance = world.entity().is_a(prefab);
      auto query = world.query_builder<const Tile>()
                       .term_at(0)
                       .second(flecs::Wildcard)
                       .build();

      THEN("the instance is found with its tileset and index") {
        int matches = 0;
        query.each([&](flecs::iter &it, size_t row, const Tile &tile) {
          if (it.entity(row) == instance) {
            ++matches;
            CHECK(it.pair(0).second() == tileset);
            CHECK(tile.index == 3);
          }
        });
        CHECK(matches == 1);
      }
    }
  }

  GIVEN("an entity already in one tileset") {
    auto other = world.entity("other");
    auto entity = world.entity().set<Tile>(tileset, {.index = 1});

    WHEN("it is moved to another tileset") {
      entity.set<Tile>(other, {.index = 2});

      THEN("it is only in the new tileset, with the new index") {
        CHECK(entity.target<Tile>() == other);
        CHECK_FALSE(entity.has<Tile>(tileset));
        CHECK(entity.get<Tile>(other).index == 2);
      }
    }
  }
}
