#include "../helpers/setup_helpers.h"
#include "modules/lua/mod_content.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

namespace {

// Counts the modifier (Modifier-bearing) children of an effect entity.
int countModifiers(flecs::entity effect) {
  int count = 0;
  effect.children([&](flecs::entity child) {
    if (child.has<Modifier>()) {
      ++count;
    }
  });
  return count;
}

} // namespace

SCENARIO("applyEffectData creates singleton effect entities",
         "[mod_content][lua]") {
  flecs::world world;
  run_effect_setup(world);

  GIVEN("an effect definition with two modifiers") {
    EffectDef struts;
    struts.id = "reinforcing_struts";
    struts.name = "Reinforcing Struts";
    struts.modifiers = {
        {.target_stat = "max-weight", .additive = 500.0, .multiplicative = 1.0},
        {.target_stat = "prep-days", .additive = 0.0, .multiplicative = 0.9},
    };

    WHEN("the definition is applied") {
      applyEffectData(world, {struts});

      THEN("a single effect entity is created under Effects keyed by id") {
        auto effect = world.lookup("Effects::reinforcing_struts");
        REQUIRE(effect.is_valid());
        CHECK(effect.has<Effect>());
        CHECK(effect.parent() == world.lookup("Effects"));
      }

      THEN("the effect is a plain entity, not a prefab") {
        auto effect = world.lookup("Effects::reinforcing_struts");
        REQUIRE(effect.is_valid());
        CHECK_FALSE(effect.has(flecs::Prefab));
      }

      THEN("the effect carries the display name as a Label") {
        auto effect = world.lookup("Effects::reinforcing_struts");
        REQUIRE(effect.is_valid());
        REQUIRE(effect.has<Label>());
        CHECK(effect.get<Label>().label == "Reinforcing Struts");
      }

      THEN("each modifier is created as a child carrying its values") {
        auto effect = world.lookup("Effects::reinforcing_struts");
        REQUIRE(effect.is_valid());
        CHECK(countModifiers(effect) == 2);

        auto first = world.lookup("Effects::reinforcing_struts::Modifier 0");
        REQUIRE(first.is_valid());
        REQUIRE(first.has<Modifier>());
        CHECK(first.get<Modifier>().target_stat == "max-weight");
        CHECK(first.get<Modifier>().additive == 500.0);
      }
    }
  }

  GIVEN("the same effect definition applied twice (a reload)") {
    EffectDef concrete;
    concrete.id = "better_concrete";
    concrete.name = "Better Concrete";
    concrete.modifiers = {
        {.target_stat = "max-weight", .additive = 0.0, .multiplicative = 1.2}};

    WHEN("the definition is applied a second time") {
      applyEffectData(world, {concrete});
      applyEffectData(world, {concrete});

      THEN("the effect entity is reused, not duplicated") {
        int count = 0;
        world.lookup("Effects").children([&](flecs::entity child) {
          if (child.name() == "better_concrete") {
            ++count;
          }
        });
        CHECK(count == 1);
      }

      THEN("its modifier children are reused, not duplicated") {
        auto effect = world.lookup("Effects::better_concrete");
        REQUIRE(effect.is_valid());
        CHECK(countModifiers(effect) == 1);
      }
    }
  }
}
