#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/lua/helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_texture", "[helpers][lua]") {
  flecs::world world;
  world.import <BaseModule>();
  registerEngineComponents(world);
  world.entity("Textures");

  GIVEN("a filename containing '..'") {
    WHEN("create_texture is called") {
      auto e = create_texture(world, TextureName{"Bad"},
                              TextureFilename{"../secret.png"},
                              TextureModName{"core"});
      THEN("an invalid entity is returned") { CHECK(!e.is_valid()); }
    }
  }

  GIVEN("a valid filename (no renderer, texture ptr will be null)") {
    WHEN("create_texture is called") {
      auto e =
          create_texture(world, TextureName{"Sheet"},
                         TextureFilename{"sheet.png"}, TextureModName{"core"});
      THEN("an entity is created as a child of Textures") {
        REQUIRE(e.is_valid());
        CHECK(e.parent() == world.lookup("Textures"));
      }
    }
  }
}
