#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/engine/render.h"
#include "modules/lua/mod_content.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("applyTilesetData creates tilesets for known textures",
         "[tileset][mod_content][lua]") {
  flecs::world world;
  world.import <BaseModule>();
  registerEngineComponents(world);
  auto textures = world.entity("Textures");
  auto roads = world.entity("roads").child_of(textures).set<Texture>(
      {.ptr = nullptr, .width = 192, .height = 32});

  GIVEN("a tileset whose tile size divides its texture") {
    TilesetDef def{.name = "roads_tiles",
                   .texture = "roads",
                   .tile_width = 32,
                   .tile_height = 32};

    WHEN("the definition is applied") {
      applyTilesetData(world, {def});

      THEN("a tileset is created under Tilesets, keyed by name") {
        auto tileset = world.lookup("Tilesets::roads_tiles");
        REQUIRE(tileset.is_valid());
        REQUIRE(tileset.has<Tileset>());
        CHECK(tileset.get<Tileset>().columns == 6);
      }

      THEN("it is linked to its texture") {
        auto tileset = world.lookup("Tilesets::roads_tiles");
        REQUIRE(tileset.is_valid());
        CHECK(tileset.target<TilesetTexture>() == roads);
      }
    }
  }

  GIVEN("a tileset whose texture does not exist") {
    TilesetDef def{.name = "missing",
                   .texture = "no_such_texture",
                   .tile_width = 32,
                   .tile_height = 32};

    WHEN("the definition is applied") {
      applyTilesetData(world, {def});

      THEN("no tileset is created") {
        CHECK_FALSE(world.lookup("Tilesets::missing").is_valid());
      }
    }
  }

  GIVEN("a tileset whose tile size does not divide its texture") {
    TilesetDef def{.name = "misfit",
                   .texture = "roads",
                   .tile_width = 50,
                   .tile_height = 32};

    WHEN("the definition is applied alongside a valid one") {
      TilesetDef valid{.name = "roads_tiles",
                       .texture = "roads",
                       .tile_width = 32,
                       .tile_height = 32};
      applyTilesetData(world, {def, valid});

      THEN("only the valid tileset is created") {
        CHECK_FALSE(world.lookup("Tilesets::misfit").is_valid());
        CHECK(world.lookup("Tilesets::roads_tiles").is_valid());
      }
    }
  }

  GIVEN("a tileset that was already applied") {
    auto markings =
        world.entity("markings")
            .child_of(textures)
            .set<Texture>({.ptr = nullptr, .width = 64, .height = 64});
    TilesetDef def{.name = "roads_tiles",
                   .texture = "roads",
                   .tile_width = 32,
                   .tile_height = 32};
    applyTilesetData(world, {def});

    WHEN("it is applied again pointing at a different texture (a reload)") {
      def.texture = "markings";
      applyTilesetData(world, {def});

      THEN("it is linked to the new texture only, with the new geometry") {
        auto tileset = world.lookup("Tilesets::roads_tiles");
        REQUIRE(tileset.is_valid());
        CHECK(tileset.target<TilesetTexture>() == markings);
        CHECK_FALSE(tileset.has<TilesetTexture>(roads));
        CHECK(tileset.get<Tileset>().columns == 2);
      }
    }
  }
}
