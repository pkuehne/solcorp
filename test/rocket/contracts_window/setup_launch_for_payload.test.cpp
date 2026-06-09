#include "modules/window/launch_window.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("buildLaunchScheduleDraft creates a launch plan from draft options",
         "[launch_window]") {
  GIVEN("draft options with a payload and target orbit") {
    flecs::world world;

    auto targetOrbit = world.entity("LEO");
    auto payload = world.entity("TestPayload");

    LaunchWindowDraftOptions options;
    options.targetOrbit = targetOrbit;
    options.payloads = {payload};

    WHEN("a launch schedule draft is built") {
      auto plan = buildLaunchScheduleDraft(options);

      THEN("the payload and target orbit are copied into the draft") {
        REQUIRE(plan.payloads[0] == payload);
        REQUIRE(plan.targetOrbit == targetOrbit);
      }
    }
  }
}
