#include "modules/site/build_mode.h"
#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>

#include <string>

namespace {

struct Rect {
  int x;
  int y;
  int w;
  int h;
  bool operator==(const Rect &) const = default;
};

Rect asRect(const PendingPlacement &placement) {
  return {.x = placement.origin.x,
          .y = placement.origin.y,
          .w = placement.footprint.w,
          .h = placement.footprint.h};
}

} // namespace

SCENARIO("dragRectangle sizes a building from a drag", "[site]") {
  const Site site{.width = 10, .height = 8};
  const FootprintLimits limits{.min = {.w = 2, .h = 2},
                               .max = {.w = 4, .h = 4}};

  struct Case {
    std::string description;
    SiteLocation anchor;
    SiteLocation hover;
    Rect expected;
  };

  GIVEN("a drag on a 10x8 site for a building of 2x2 to 4x4") {
    auto [description, anchor, hover, expected] = GENERATE(values<Case>({
        {"no movement gives the minimum",
         {.x = 5, .y = 5},
         {.x = 5, .y = 5},
         {.x = 5, .y = 5, .w = 2, .h = 2}},
        {"right and down moves the right and bottom edges",
         {.x = 2, .y = 2},
         {.x = 4, .y = 3},
         {.x = 2, .y = 2, .w = 3, .h = 2}},
        {"left and up moves only the left and top edges",
         {.x = 5, .y = 5},
         {.x = 3, .y = 3},
         {.x = 3, .y = 3, .w = 4, .h = 4}},
        {"left by one keeps the cell right of the press",
         {.x = 3, .y = 2},
         {.x = 2, .y = 2},
         {.x = 2, .y = 2, .w = 3, .h = 2}},
        {"back over the press shrinks to the minimum",
         {.x = 3, .y = 2},
         {.x = 3, .y = 2},
         {.x = 3, .y = 2, .w = 2, .h = 2}},
        {"past the maximum to the right holds the right edge back",
         {.x = 1, .y = 1},
         {.x = 8, .y = 1},
         {.x = 1, .y = 1, .w = 4, .h = 2}},
        {"past the maximum to the left holds the left edge back",
         {.x = 8, .y = 1},
         {.x = 1, .y = 1},
         {.x = 6, .y = 1, .w = 4, .h = 2}},
        {"pressed in the last column it is cut below the minimum",
         {.x = 9, .y = 3},
         {.x = 9, .y = 3},
         {.x = 9, .y = 3, .w = 1, .h = 2}},
        {"pressed in the last column and dragged left it fits",
         {.x = 9, .y = 3},
         {.x = 8, .y = 3},
         {.x = 8, .y = 3, .w = 2, .h = 2}},
        {"along the bottom edge it is cut below the minimum",
         {.x = 2, .y = 7},
         {.x = 5, .y = 7},
         {.x = 2, .y = 7, .w = 4, .h = 1}},
        {"towards the top-left corner",
         {.x = 1, .y = 1},
         {.x = 0, .y = 0},
         {.x = 0, .y = 0, .w = 3, .h = 3}},
    }));
    CAPTURE(description);

    THEN("the rectangle is as expected") {
      CHECK(asRect(dragRectangle(anchor, hover, limits, site)) == expected);
    }
  }
}

SCENARIO("pendingPlacement is what a release would place",
         "[build_mode][site]") {
  flecs::world world;
  world.import <SiteModule>();
  const Site site{.width = 10, .height = 8};
  auto hall = world.prefab().set<FootprintLimits>(
      {.min = {.w = 3, .h = 2}, .max = {.w = 5, .h = 5}});
  auto gate = world.prefab().add<FixedSize>();

  struct Case {
    std::string description;
    bool fixed_size;
    bool dragging;
    Rect expected;
  };

  GIVEN("a hover at (6, 4) after pressing at (2, 2)") {
    auto [description, fixed_size, dragging, expected] = GENERATE(values<Case>({
        {"dragging a draggable building",
         false,
         true,
         {.x = 2, .y = 2, .w = 5, .h = 3}},
        {"not dragging a draggable building",
         false,
         false,
         {.x = 6, .y = 4, .w = 3, .h = 2}},
        {"dragging a fixed-size building",
         true,
         true,
         {.x = 6, .y = 4, .w = 1, .h = 1}},
    }));
    CAPTURE(description);
    const BuildMode mode{.kind = PlacementKind::Building,
                         .drag_start = {.x = 2, .y = 2},
                         .dragging = dragging};

    THEN("the placement is as expected") {
      CHECK(asRect(pendingPlacement(fixed_size ? gate : hall, mode,
                                    {.x = 6, .y = 4}, site)) == expected);
    }
  }
}

SCENARIO("dragRun lays a straight road from a drag", "[site]") {
  struct Case {
    std::string description;
    SiteLocation anchor;
    SiteLocation hover;
    Rect expected;
  };

  GIVEN("a drag") {
    auto [description, anchor, hover, expected] = GENERATE(values<Case>({
        {"no movement is one cell",
         {.x = 3, .y = 3},
         {.x = 3, .y = 3},
         {.x = 3, .y = 3, .w = 1, .h = 1}},
        {"right",
         {.x = 3, .y = 3},
         {.x = 6, .y = 3},
         {.x = 3, .y = 3, .w = 4, .h = 1}},
        {"left",
         {.x = 3, .y = 3},
         {.x = 1, .y = 3},
         {.x = 1, .y = 3, .w = 3, .h = 1}},
        {"down",
         {.x = 3, .y = 3},
         {.x = 3, .y = 5},
         {.x = 3, .y = 3, .w = 1, .h = 3}},
        {"up",
         {.x = 3, .y = 3},
         {.x = 3, .y = 0},
         {.x = 3, .y = 0, .w = 1, .h = 4}},
        {"mostly sideways snaps to the press row",
         {.x = 3, .y = 3},
         {.x = 7, .y = 5},
         {.x = 3, .y = 3, .w = 5, .h = 1}},
        {"mostly vertical snaps to the press column",
         {.x = 3, .y = 3},
         {.x = 2, .y = 7},
         {.x = 3, .y = 3, .w = 1, .h = 5}},
        {"a diagonal tie runs sideways",
         {.x = 3, .y = 3},
         {.x = 5, .y = 5},
         {.x = 3, .y = 3, .w = 3, .h = 1}},
    }));
    CAPTURE(description);

    THEN("the run is as expected") {
      CHECK(asRect(dragRun(anchor, hover)) == expected);
    }
  }
}
