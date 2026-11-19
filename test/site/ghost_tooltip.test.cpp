#include "modules/base/base.h"
#include "modules/site/build_mode.h"
#include "modules/site/connector.h"
#include "modules/site/placement.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <flecs.h>

#include <string>

SCENARIO("ghostTooltip describes what a release would build",
         "[build_mode][site]") {
  flecs::world world;
  world.import <SiteModule>();
  world.prefab("factory")
      .set<Label>({"Factory"})
      .set<FootprintLimits>({.min = {.w = 2, .h = 2}, .max = {.w = 4, .h = 5}});
  world.prefab("yard").set<Label>({"Yard"});
  world.prefab("gate").add<FixedSize>();
  world.prefab("road").set<Label>({"Road"}).set<ConnectorTile>({});

  struct Case {
    std::string description;
    std::string prefab;
    bool dragging;
    Footprint footprint;
    std::string problem;
    GhostTooltip expected;
  };

  GIVEN("a build ghost") {
    auto [description, prefab, dragging, footprint, problem, expected] =
        GENERATE(values<Case>({
            {"a draggable building before a drag",
             "factory",
             false,
             {.w = 2, .h = 2},
             "",
             {.title = "Factory",
              .size = "Size 2 × 2 (max 4 × 5)",
              .problem = "",
              .hint = "Click and drag to size · Right-click to cancel"}},
            {"a draggable building mid-drag",
             "factory",
             true,
             {.w = 3, .h = 4},
             "",
             {.title = "Factory",
              .size = "Size 3 × 4 (max 4 × 5)",
              .problem = "",
              .hint = "Release to build · Right-click to cancel"}},
            {"an invalid placement",
             "factory",
             true,
             {.w = 3, .h = 1},
             "It is too small",
             {.title = "Factory",
              .size = "Size 3 × 1 (max 4 × 5)",
              .problem = "It is too small",
              .hint = "Release to build · Right-click to cancel"}},
            {"a building without a maximum",
             "yard",
             false,
             {.w = 2, .h = 2},
             "",
             {.title = "Yard",
              .size = "Size 2 × 2",
              .problem = "",
              .hint = "Click and drag to size · Right-click to cancel"}},
            {"a road",
             "road",
             false,
             {.w = 1, .h = 1},
             "",
             {.title = "Road",
              .size = "Size 1 \u00d7 1",
              .problem = "",
              .hint = "Click to build \u00b7 Right-click to cancel"}},
            {"a fixed-size building without a label",
             "gate",
             false,
             {.w = 1, .h = 1},
             "",
             {.title = "gate",
              .size = "Size 1 × 1",
              .problem = "",
              .hint = "Click to build · Right-click to cancel"}},
        }));
    CAPTURE(description);
    const BuildMode mode{.kind = PlacementKind::Building,
                         .drag_start = {},
                         .dragging = dragging};
    const PendingPlacement placement{.origin = {}, .footprint = footprint};
    const auto validation = ValidationResult::Issue(problem);

    WHEN("its tooltip is built") {
      const auto tooltip = ghostTooltip(world.lookup(prefab.c_str()), mode,
                                        placement, validation);

      THEN("every line is as expected") {
        CHECK(tooltip.title == expected.title);
        CHECK(tooltip.size == expected.size);
        CHECK(tooltip.problem == expected.problem);
        CHECK(tooltip.hint == expected.hint);
      }
    }
  }
}
