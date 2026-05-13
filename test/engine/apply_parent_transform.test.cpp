#include "modules/engine/render.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

void systemApplyParentTransform(Transform &t, const Transform *parent);

SCENARIO("systemApplyParentTransform", "[system]") {
  GIVEN("A point at (10,20)") {
    Point original{.x = 10, .y = 20};
    WHEN("A transform is applied without a parent") {
      Transform t{.relativePosition = original, .worldPosition = {}};

      systemApplyParentTransform(t, nullptr);

      THEN("The world position is the same as the relative position") {
        REQUIRE(t.relativePosition.x == original.x);
        REQUIRE(t.relativePosition.y == original.y);
        REQUIRE(t.worldPosition.x == original.x);
        REQUIRE(t.worldPosition.y == original.y);
      }
    }

    WHEN("A transform is applied with a parent at (10,20)") {
      Transform t{.relativePosition = original, .worldPosition = {}};
      Transform p{.relativePosition = original, .worldPosition = original};

      systemApplyParentTransform(t, &p);

      THEN("The world position is the sum of the relative and parent "
           "positions") {
        REQUIRE(t.relativePosition.x == original.x);
        REQUIRE(t.relativePosition.y == original.y);
        REQUIRE(t.worldPosition.x == original.x * 2);
        REQUIRE(t.worldPosition.y == original.y * 2);
      }
    }
  }
}
