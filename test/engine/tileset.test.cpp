#include "modules/engine/render.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>

SCENARIO("makeTileset", "[tileset]") {
  struct ValidCase {
    int texture_width;
    int texture_height;
    int tile_width;
    int tile_height;
    int expected_columns;
  };

  GIVEN("A tile size that evenly divides the texture") {
    auto [texture_width, texture_height, tile_width, tile_height,
          expected_columns] = GENERATE(values<ValidCase>({
        {192, 32, 32, 32, 6}, // horizontal strip
        {96, 96, 32, 32, 3},  // square grid
        {64, 48, 16, 24, 4},  // non-square tiles
        {32, 32, 32, 32, 1},  // single tile
    }));
    CAPTURE(texture_width, texture_height, tile_width, tile_height);
    const Texture texture{
        .ptr = nullptr, .width = texture_width, .height = texture_height};

    WHEN("A tileset is made from them") {
      const Tileset tileset = makeTileset(texture, tile_width, tile_height);

      THEN("It has the tile size and the derived column count") {
        CHECK(tileset.tile_width == tile_width);
        CHECK(tileset.tile_height == tile_height);
        CHECK(tileset.columns == expected_columns);
      }
    }
  }

  struct InvalidCase {
    int texture_width;
    int texture_height;
    int tile_width;
    int tile_height;
    std::string expected_message;
  };

  GIVEN("A tile size that does not fit the texture") {
    auto [texture_width, texture_height, tile_width, tile_height,
          expected_message] = GENERATE(values<InvalidCase>({
        {100, 32, 32, 32,
         "tile size 32x32 does not evenly divide the 100x32 texture"},
        {96, 40, 32, 32,
         "tile size 32x32 does not evenly divide the 96x40 texture"},
        {16, 16, 32, 32, "tile size 32x32 is larger than the 16x16 texture"},
        {0, 0, 32, 32,
         "tile size 32x32 is larger than the 0x0 texture"}, // failed load
        {96, 96, 0, 32, "tile size 0x32 is not positive"},
        {96, 96, 32, -32, "tile size 32x-32 is not positive"},
    }));
    CAPTURE(texture_width, texture_height, tile_width, tile_height);
    const Texture texture{
        .ptr = nullptr, .width = texture_width, .height = texture_height};

    THEN("Making a tileset throws, saying why") {
      CHECK_THROWS_MATCHES(makeTileset(texture, tile_width, tile_height),
                           std::invalid_argument,
                           Catch::Matchers::Message(expected_message));
    }
  }
}

SCENARIO("tileRect", "[tileset]") {
  struct Case {
    Tileset tileset;
    uint32_t index;
    SDL_Rect expected;
  };

  constexpr Tileset strip{.tile_width = 32, .tile_height = 32, .columns = 6};
  constexpr Tileset grid{.tile_width = 32, .tile_height = 32, .columns = 3};
  constexpr Tileset tall{.tile_width = 16, .tile_height = 24, .columns = 4};

  GIVEN("A tileset and a tile index") {
    auto [tileset, index, expected] = GENERATE_COPY(values<Case>({
        {strip, 0, {0, 0, 32, 32}},
        {strip, 5, {160, 0, 32, 32}},
        {grid, 2, {64, 0, 32, 32}},  // last column of the first row
        {grid, 3, {0, 32, 32, 32}},  // wraps to the second row
        {grid, 8, {64, 64, 32, 32}}, // last tile
        {tall, 5, {16, 24, 16, 24}}, // non-square tiles
    }));
    CAPTURE(tileset.columns, index);

    WHEN("Its source rect is requested") {
      const SDL_Rect rect = tileRect(tileset, index);

      THEN("It is the tile's position and size in the texture") {
        CHECK(rect.x == expected.x);
        CHECK(rect.y == expected.y);
        CHECK(rect.w == expected.w);
        CHECK(rect.h == expected.h);
      }
    }
  }
}
