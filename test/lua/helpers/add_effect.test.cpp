#include "modules/lua/helpers.h"
#include "modules/lua/mod_content.h"
#include "modules/stats/stats.h"
#include "setup_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("add_effect", "[helpers][lua]") {
  flecs::world world;
  run_effect_setup(world);

  // A single effect, created once under Effects by the data loader.
  EffectDef concrete;
  concrete.id = "better_concrete";
  concrete.name = "Better Concrete";
  concrete.modifiers = {
      {.target_stat = "max-weight", .additive = 0.0, .multiplicative = 1.2}};
  applyEffectData(world, {concrete});

  GIVEN("an existing effect id and a source entity") {
    auto source = world.entity("Launchpad");

    WHEN("add_effect is called") {
      auto effect = add_effect(world, source, "better_concrete");

      THEN("the returned effect is the singleton under Effects") {
        REQUIRE(effect.is_valid());
        CHECK(effect == world.lookup("Effects::better_concrete"));
      }
      THEN("the source references the effect via HasEffect") {
        CHECK(source.has<HasEffect>(effect));
      }
    }

    WHEN("two sources attach the same effect") {
      auto other = world.entity("Pad B");
      auto a = add_effect(world, source, "better_concrete");
      auto b = add_effect(world, other, "better_concrete");

      THEN("both reference the same single effect entity") {
        CHECK(a == b);
        CHECK(source.has<HasEffect>(a));
        CHECK(other.has<HasEffect>(a));
      }
    }
  }

  GIVEN("an effect id that does not exist") {
    auto source = world.entity("Launchpad");

    WHEN("add_effect is called") {
      auto effect = add_effect(world, source, "no_such_effect");

      THEN("an invalid entity is returned and no relationship is added") {
        CHECK(!effect.is_valid());
        CHECK(!source.has<HasEffect>(flecs::Wildcard));
      }
    }
  }
}
