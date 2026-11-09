#include "modules/engine/render.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

SCENARIO("containsPoint", "[site]") {
  struct Case {
    Point point;
    bool expected;
  };

  GIVEN("a 32x16 rect at (100, 200)") {
    constexpr Point origin{.x = 100, .y = 200};
    auto [point, expected] = GENERATE(values<Case>({
        {{.x = 116, .y = 208}, true},  // centre
        {{.x = 101, .y = 201}, true},  // just inside top-left
        {{.x = 131, .y = 215}, true},  // just inside bottom-right
        {{.x = 100, .y = 208}, false}, // on the left edge
        {{.x = 132, .y = 208}, false}, // on the right edge
        {{.x = 116, .y = 200}, false}, // on the top edge
        {{.x = 116, .y = 216}, false}, // on the bottom edge
        {{.x = 140, .y = 208}, false}, // right of the rect
        {{.x = 116, .y = 220}, false}, // below: height, not width, bounds y
    }));
    CAPTURE(point.x, point.y);

    THEN("only points strictly inside it are contained") {
      CHECK(containsPoint(origin, 32, 16, point) == expected);
    }
  }
}
