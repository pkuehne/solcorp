#include "modules/stats/stats.h"
#include "modules/base/base.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Stats", "[stats]") {
  GIVEN("A test stat with value 10") {
    Stat stat{{.id = "test_stat",
               .display = "Displ",
               .description = "More Text",
               .base = 10.0}};

    THEN("The base value is 10") { REQUIRE(stat.base() == 10.0); }

    WHEN("A multiplier mod of 2 is applied") {
      Modifier mod = {
          .target_stat = "test_stat", .additive = 0.0, .multiplicative = 2.0};
      stat.addModifier(mod, "");
      THEN("The result is 20") { REQUIRE(stat.value() == 20.0); }
    }

    WHEN("An adder mod of 10 is applied") {
      Modifier mod = {
          .target_stat = "test_stat", .additive = 10.0, .multiplicative = 1.0};
      stat.addModifier(mod, "");
      THEN("The result is 20") { REQUIRE(stat.value() == 20.0); }
    }

    WHEN("An adder mod of 10 is applied and then reset") {
      Modifier mod = {
          .target_stat = "test_stat", .additive = 10.0, .multiplicative = 1.0};
      bool result = stat.addModifier(mod, "");
      stat.reset();

      THEN("The result is again 10") {
        REQUIRE(result == true);
        REQUIRE(stat.value() == 10.0);
      }
    }

    WHEN("An unrelated modifier is added") {
      Modifier mod = {
          .target_stat = "other_stat", .additive = 10.0, .multiplicative = 1.0};
      bool result = stat.addModifier(mod, "");
      THEN("The result is again 10") {
        REQUIRE(result == false);
        REQUIRE(stat.value() == 10.0);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// discover_stat_update_systems
// ---------------------------------------------------------------------------

namespace {

// Attach an effect with a single additive modifier to target.
void add_effect(flecs::world &world, flecs::entity target,
                const std::string &stat_id, double additive) {
  auto effect = world.entity();
  world.entity()
      .child_of(effect)
      .set<Modifier>({.target_stat = stat_id,
                      .additive = additive,
                      .multiplicative = 1.0});
  target.add<HasEffect>(effect);
}

} // namespace

SCENARIO("discover_stat_update_systems — component with no Stat members",
         "[stats][discover]") {
  GIVEN("A world with StatsModule and a component that has no Stat members") {
    flecs::world world;
    world.import<BaseModule>();
    world.import<StatsModule>();

    struct NoStatComp {
      int value = 0;
    };
    world.component<NoStatComp>().member("value", &NoStatComp::value);

    WHEN("discover_stat_update_systems is called") {
      // Should not register any system for NoStatComp and must not crash.
      discover_stat_update_systems(world);

      THEN("The world still progresses without error") {
        REQUIRE_NOTHROW(world.progress());
      }
    }
  }
}

SCENARIO("discover_stat_update_systems — single Stat member updated",
         "[stats][discover]") {
  GIVEN("A world with a component containing one Stat member (base = 10)") {
    flecs::world world;
    world.import<BaseModule>();
    world.import<StatsModule>();

    struct SingleStatComp {
      Stat skill = Stat({.id = "skill",
                         .display = "Skill",
                         .description = "Test skill",
                         .base = 10.0});
    };
    world.component<SingleStatComp>().member("skill",
                                             &SingleStatComp::skill);

    auto e = world.entity().set<SingleStatComp>({});
    add_effect(world, e, "skill", 5.0); // additive +5

    WHEN("discover_stat_update_systems is called and UpdatePhase runs") {
      discover_stat_update_systems(world);
      world.progress();

      THEN("The modifier is applied: value == 15") {
        REQUIRE(e.get<SingleStatComp>().skill.value() == 15.0);
      }
    }
  }
}

SCENARIO("discover_stat_update_systems — multiple Stat members updated",
         "[stats][discover]") {
  GIVEN("A world with a component containing two Stat members") {
    flecs::world world;
    world.import<BaseModule>();
    world.import<StatsModule>();

    struct MultiStatComp {
      Stat alpha = Stat({.id = "alpha",
                         .display = "Alpha",
                         .description = "First stat",
                         .base = 100.0});
      Stat beta = Stat({.id = "beta",
                        .display = "Beta",
                        .description = "Second stat",
                        .base = 50.0});
    };
    world.component<MultiStatComp>()
        .member("alpha", &MultiStatComp::alpha)
        .member("beta", &MultiStatComp::beta);

    auto e = world.entity().set<MultiStatComp>({});
    add_effect(world, e, "alpha", 10.0);  // alpha += 10
    add_effect(world, e, "beta", -20.0); // beta -= 20

    WHEN("discover_stat_update_systems is called and UpdatePhase runs") {
      discover_stat_update_systems(world);
      world.progress();

      THEN("Both stats receive their respective modifiers") {
        const auto &comp = e.get<MultiStatComp>();
        REQUIRE(comp.alpha.value() == 110.0);
        REQUIRE(comp.beta.value() == 30.0);
      }
    }
  }
}

SCENARIO("discover_stat_update_systems — modifiers reset on subsequent ticks",
         "[stats][discover]") {
  GIVEN("A component with a Stat and a persistent effect") {
    flecs::world world;
    world.import<BaseModule>();
    world.import<StatsModule>();

    struct PersistComp {
      Stat capacity = Stat({.id = "capacity",
                             .display = "Capacity",
                             .description = "Storage capacity",
                             .base = 200.0});
    };
    world.component<PersistComp>().member("capacity", &PersistComp::capacity);

    auto e = world.entity().set<PersistComp>({});
    add_effect(world, e, "capacity", 50.0); // +50

    discover_stat_update_systems(world);
    world.progress(); // first tick applies modifier

    WHEN("A second tick runs") {
      world.progress();

      THEN("The value is still 250 (not double-accumulated)") {
        REQUIRE(e.get<PersistComp>().capacity.value() == 250.0);
      }
    }
  }
}

SCENARIO("discover_stat_update_systems — component without .member() call "
         "is ignored",
         "[stats][discover]") {
  GIVEN("A component registered without any .member() reflection") {
    flecs::world world;
    world.import<BaseModule>();
    world.import<StatsModule>();

    struct UnreflectedComp {
      Stat hidden = Stat({.id = "hidden",
                           .display = "Hidden",
                           .description = "Not reflected",
                           .base = 5.0});
    };
    // Deliberately NOT calling .member() here.
    world.component<UnreflectedComp>();

    auto e = world.entity().set<UnreflectedComp>({});
    add_effect(world, e, "hidden", 99.0);

    WHEN("discover_stat_update_systems is called and UpdatePhase runs") {
      discover_stat_update_systems(world);
      world.progress();

      THEN("The hidden stat is NOT updated (no updater was registered)") {
        // Modifier is not applied because the component has no struct metadata.
        REQUIRE(e.get<UnreflectedComp>().hidden.value() == 5.0);
      }
    }
  }
}
