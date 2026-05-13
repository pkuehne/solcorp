#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/lua/helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("clip_sprite_from_texture", "[helpers][lua]") {
  flecs::world world;
  world.import<BaseModule>();
  registerEngineComponents(world);

  GIVEN("a texture name that does not exist") {
    WHEN("clip_sprite_from_texture is called") {
      auto sprite = clip_sprite_from_texture(
          world, "missing",
          SpriteClipRect{.x = 0, .y = 0, .width = 32, .height = 32});
      THEN("an empty Sprite is returned") {
        CHECK(sprite.texture == flecs::entity());
      }
    }
  }

  GIVEN("a texture entity exists in the Textures hierarchy") {
    auto textures = world.entity("Textures");
    auto tex = world.entity("sheet").child_of(textures);

    WHEN("clip_sprite_from_texture is called with a clipping region") {
      auto sprite = clip_sprite_from_texture(
          world, "sheet",
          SpriteClipRect{.x = 10, .y = 20, .width = 64, .height = 48});
      THEN("the sprite references the texture entity") {
        CHECK(sprite.texture == tex);
      }
      THEN("the clip coordinates are set correctly") {
        CHECK(sprite.x == 10);
        CHECK(sprite.y == 20);
        CHECK(sprite.width == 64);
        CHECK(sprite.height == 48);
      }
    }
  }
}
