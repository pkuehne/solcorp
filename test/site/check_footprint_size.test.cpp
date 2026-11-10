#include "modules/site/placement.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

SCENARIO("checkFootprintSize", "[site][placement]") {
  struct Case {
    FootprintLimits limits;
    Footprint footprint;
    PlacementResult expected;
  };

  constexpr FootprintLimits any_size{};
  constexpr FootprintLimits office{.min = {.w = 2, .h = 4},
                                   .max = {.w = 6, .h = 8}};
  constexpr FootprintLimits below_floor{.min = {.w = 1, .h = 1},
                                        .max = {.w = 3, .h = 3}};

  GIVEN("a building's size limits and a dragged footprint") {
    auto [limits, footprint, expected] = GENERATE_COPY(values<Case>({
        {any_size, {.w = 2, .h = 2}, PlacementResult::Ok},
        {any_size, {.w = 40, .h = 30}, PlacementResult::Ok},
        {any_size, {.w = 1, .h = 3}, PlacementResult::TooSmall},
        {any_size, {.w = 3, .h = 1}, PlacementResult::TooSmall},
        {office, {.w = 2, .h = 4}, PlacementResult::Ok},
        {office, {.w = 6, .h = 8}, PlacementResult::Ok},
        {office, {.w = 4, .h = 2}, PlacementResult::TooSmall}, // no rotation
        {office, {.w = 2, .h = 3}, PlacementResult::TooSmall},
        {office, {.w = 7, .h = 8}, PlacementResult::TooLarge},
        {office, {.w = 6, .h = 9}, PlacementResult::TooLarge},
        {below_floor, {.w = 1, .h = 1}, PlacementResult::TooSmall}, // 2x2 floor
        {below_floor, {.w = 2, .h = 2}, PlacementResult::Ok},
    }));
    CAPTURE(limits.min.w, limits.min.h, limits.max.w, limits.max.h, footprint.w,
            footprint.h);

    THEN("it is accepted only within the limits and the 2x2 floor") {
      CHECK(checkFootprintSize(footprint, limits) == expected);
    }
  }
}
