#include "modules/engine/input.h"
#include "modules/engine/render.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>

SCENARIO("centredSiteOrigin", "[site]") {
  struct Case {
    int window_width;
    int window_height;
    Site site;
    Point expected;
  };

  GIVEN("a window size and a site") {
    auto [window_width, window_height, site, expected] = GENERATE(values<Case>({
        {1000, 800, {.width = 10, .height = 10}, {.x = 340, .y = 240}},
        {1000, 800, {.width = 20, .height = 20}, {.x = 180, .y = 80}},
        {1920, 1080, {.width = 20, .height = 10}, {.x = 640, .y = 380}},
        {641, 641, {.width = 20, .height = 20}, {.x = 0.5F, .y = 0.5F}},
        {320, 200, {.width = 20, .height = 10}, {.x = -160, .y = -60}},
    }));
    CAPTURE(window_width, window_height, site.width, site.height);

    THEN("the origin leaves equal space on both sides of the site") {
      const Point origin = centredSiteOrigin(window_width, window_height, site);
      CHECK(origin.x == expected.x);
      CHECK(origin.y == expected.y);
    }
  }
}

SCENARIO("Sites stay centred in the window", "[site]") {
  flecs::world world;
  world.import <SiteModule>();

  GIVEN("a 1000x800 window") {
    world.set<WindowSize>({.width = 1000, .height = 800});

    WHEN("a 10x10 site is created") {
      auto site = world.entity()
                      .set<Site>({.width = 10, .height = 10})
                      .set<Transform>({});

      THEN("it is centred in the window") {
        const auto &position = site.get<Transform>().relativePosition;
        CHECK(position.x == 340);
        CHECK(position.y == 240);
      }

      AND_WHEN("the window is resized to 1920x1080") {
        world.set<WindowSize>({.width = 1920, .height = 1080});

        THEN("it is re-centred in the new size") {
          const auto &position = site.get<Transform>().relativePosition;
          CHECK(position.x == 800);
          CHECK(position.y == 380);
        }
      }
    }
  }

  GIVEN("no window (e.g. before graphics start)") {
    WHEN("a site is created") {
      auto site = world.entity()
                      .set<Site>({.width = 10, .height = 10})
                      .set<Transform>({.relativePosition = {.x = 5, .y = 6},
                                       .worldPosition = {}});

      THEN("it is left where it was put") {
        const auto &position = site.get<Transform>().relativePosition;
        CHECK(position.x == 5);
        CHECK(position.y == 6);
      }
    }
  }
}
