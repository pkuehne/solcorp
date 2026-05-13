#include "modules/lua/helpers.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("create_effect", "[helpers][lua]") {
  flecs::world world;
  world.import<StatsModule>();

  GIVEN("a name with no source entity") {
    WHEN("create_effect is called") {
      auto effect = create_effect(world, "BurnEffect", flecs::entity());
      THEN("a valid entity with Effect component is created") {
        REQUIRE(effect.is_valid());
        CHECK(effect.has<Effect>());
      }
      THEN("effect is a child of the Effects node") {
        CHECK(effect.parent() == world.lookup("Effects"));
      }
    }
  }

  GIVEN("a name and a valid source entity") {
    WHEN("create_effect is called") {
      auto source = world.entity("Booster");
      auto effect = create_effect(world, "ThrustEffect", source);
      THEN("source has HasEffect pointing to the new effect") {
        REQUIRE(effect.is_valid());
        CHECK(source.has<HasEffect>(effect));
      }
    }
  }
}
