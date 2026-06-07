#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>

namespace {

struct ReflectedStats {
  Stat thrust{{.id = "thrust", .base = 100.0}};
  Stat cost{{.id = "cost", .base = 1000.0}};
  Stat hidden{{.id = "hidden", .base = 5.0}};
};

} // namespace

SCENARIO("Stats", "[stats]") {
  GIVEN("A test stat with value 10") {
    Stat stat{{.id = "test_stat", .base = 10.0}};

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

SCENARIO("Reflected stat registry", "[stats]") {
  flecs::world world;
  world.import <StatsModule>();

  world.component<ReflectedStats>()
      .member("thrust", &ReflectedStats::thrust)
      .member("cost", &ReflectedStats::cost);

  GIVEN("an entity with a reflected stats component and matching effects") {
    auto source = world.entity("Source");
    auto effect = world.entity("Engine Upgrade").add<Effect>();
    world.entity()
        .child_of(effect)
        .set<Modifier>(
            {.target_stat = "thrust", .additive = 25.0, .multiplicative = 1.0});
    world.entity()
        .child_of(effect)
        .set<Modifier>(
            {.target_stat = "cost", .additive = 250.0, .multiplicative = 1.0});
    source.add<HasEffect>(effect);

    auto entity = world.entity("Vehicle").child_of(source).set<ReflectedStats>(
        {});

    WHEN("the world progresses") {
      world.progress();

      THEN("all reflected stats receive their modifiers automatically") {
        const auto &stats = entity.get<ReflectedStats>();
        CHECK(stats.thrust.value() == 125.0);
        CHECK(stats.cost.value() == 1250.0);
      }

      THEN("unregistered stat members are not discovered") {
        const auto &stats = entity.get<ReflectedStats>();
        CHECK(stats.hidden.value() == 5.0);
        CHECK(findStat(entity, "hidden") == nullptr);
      }
    }

    WHEN("a stat is found by id") {
      world.progress();
      auto *stat = findStat(entity, "cost");

      THEN("the reflected stat pointer is returned") {
        REQUIRE(stat != nullptr);
        CHECK(stat->id() == "cost");
      }
    }
  }
}

SCENARIO("Stat definitions", "[stats]") {
  flecs::world world;
  world.import <StatsModule>();

  GIVEN("the stats module is imported") {
    THEN("StatDef and its format enum are reflected") {
      const auto statDef = world.component<StatDef>();
      const auto statFormat = world.component<StatDef::Format>();

      CHECK(statDef.has<EcsStruct>());
      CHECK(statFormat.has<EcsEnum>());
      CHECK(statFormat.lookup("Number").is_valid());
      CHECK(statFormat.lookup("Currency").is_valid());
      CHECK(statFormat.lookup("Percentage").is_valid());
    }
  }

  GIVEN("a registered stat definition") {
    registerStatDef(world, {.id = "failure-rate",
                            .display = "Failure Rate",
                            .description = "Chance of launch failure",
                            .higher_is_better = false,
                            .format = Stat::Format::Percentage});

    THEN("it can be looked up without a live stat instance") {
      const auto *definition = findStatDef(world, "failure-rate");
      REQUIRE(definition != nullptr);
      CHECK(definition->display == "Failure Rate");
      CHECK(definition->higher_is_better == false);
      CHECK(definition->formatValue(0.1) == "10%");
    }
  }

  GIVEN("an unknown stat id") {
    THEN("a default display definition is available") {
      const auto definition = statDef(world, "modded-stat");
      CHECK(definition.id == "modded-stat");
      CHECK(definition.display == "modded-stat");
      CHECK(definition.description.empty());
    }
  }
}

SCENARIO("Stat formatting", "[stats]") {
  GIVEN("a default number stat definition") {
    StatDef definition{.id = "count",
                       .display = "Count",
                       .description = "A plain number"};

    THEN("it formats as a rounded number") {
      REQUIRE(definition.formatValue(12.4) == "12");
    }
  }

  GIVEN("a currency stat definition") {
    StatDef definition{.id = "cost",
                       .display = "Cost",
                       .description = "A money value",
                       .format = Stat::Format::Currency};

    THEN("it formats with a dollar prefix and separators") {
      REQUIRE(definition.formatValue(1250) == "$1,250");
    }
  }

  GIVEN("a percentage stat definition") {
    StatDef definition{.id = "failure-rate",
                       .display = "Failure Rate",
                       .description = "A probability",
                       .format = Stat::Format::Percentage};

    THEN("it formats probabilities as whole percentages") {
      REQUIRE(definition.formatValue(0.1) == "10%");
    }
  }
}
