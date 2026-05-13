#include "modules/window/building_detail_window.h"
#include "modules/base/base.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <flecs.h>

SCENARIO("getEntityEffortRequired", "[window]") {
  flecs::world world;
  world.component<EffortRequired>();

  GIVEN("An entity with no EffortRequired component") {
    auto entity = world.entity();
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 1.0 (nothing to wait for)") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(1.0f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired fully remaining") {
    auto entity = world.entity().set<EffortRequired>({.remaining = 100, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 0.0 (no progress)") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.0f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired fully complete") {
    auto entity = world.entity().set<EffortRequired>({.remaining = 0, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 1.0 (fully done)") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(1.0f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired halfway complete") {
    auto entity = world.entity().set<EffortRequired>({.remaining = 50, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 0.5") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.5f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired at one tick remaining") {
    auto entity = world.entity().set<EffortRequired>({.remaining = 1, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 0.99") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.99f, 0.001f));
      }
    }
  }
}
