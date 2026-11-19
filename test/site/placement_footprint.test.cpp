#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>

#include <optional>
#include <string>

SCENARIO("placementFootprint is the size a click places", "[site]") {
  struct Case {
    std::string description;
    bool fixed_size;
    std::optional<Footprint> min;
    Footprint expected;
  };

  GIVEN("a building prefab") {
    auto [description, fixed_size, min, expected] = GENERATE(values<Case>({
        {"a fixed-size building", true, std::nullopt, {.w = 1, .h = 1}},
        {"a draggable building without limits",
         false,
         std::nullopt,
         {.w = 2, .h = 2}},
        {"a draggable building with a larger minimum",
         false,
         Footprint{.w = 3, .h = 4},
         {.w = 3, .h = 4}},
        {"a minimum below 2x2 in one dimension",
         false,
         Footprint{.w = 1, .h = 3},
         {.w = 2, .h = 3}},
    }));
    CAPTURE(description);

    flecs::world world;
    world.import <SiteModule>();
    auto prefab = world.prefab();
    if (fixed_size) {
      prefab.add<FixedSize>();
    }
    if (min) {
      prefab.set<FootprintLimits>({.min = *min});
    }

    WHEN("its placement footprint is requested") {
      const Footprint footprint = placementFootprint(prefab);

      THEN("it is the expected size") {
        CHECK(footprint.w == expected.w);
        CHECK(footprint.h == expected.h);
      }
    }
  }
}
