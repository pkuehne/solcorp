#include "modules/lua/helpers.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("add_modifier", "[helpers][lua]") {
  flecs::world world;
  world.import<StatsModule>();

  GIVEN("a valid effect entity") {
    WHEN("add_modifier is called") {
      auto effect = world.entity("SpeedEffect").add<Effect>();
      Modifier mod{
          .target_stat = "thrust", .additive = 100.0, .multiplicative = 1.5};
      auto modifier = add_modifier(world, effect, mod);
      THEN("a valid modifier entity is returned") {
        REQUIRE(modifier.is_valid());
        CHECK(modifier.has<Modifier>());
      }
      THEN("modifier is a child of the effect") {
        CHECK(modifier.parent() == effect);
      }
      THEN("modifier data is stored correctly") {
        auto &m = modifier.get<Modifier>();
        CHECK(m.target_stat == "thrust");
        CHECK(m.additive == 100.0);
        CHECK(m.multiplicative == 1.5);
      }
    }
  }

  GIVEN("an invalid effect entity") {
    WHEN("add_modifier is called") {
      auto modifier = add_modifier(world, flecs::entity(), Modifier{});
      THEN("an invalid entity is returned") { CHECK(!modifier.is_valid()); }
    }
  }
}
