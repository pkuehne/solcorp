#include "modules/base/base.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/site/weather.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Weather pattern entities", "[weather]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();

  GIVEN("The world is initialised") {
    THEN("All five weather pattern entities exist") {
      CHECK(world.lookup("Patterns::Weather::Clear").is_valid());
      CHECK(world.lookup("Patterns::Weather::PartlyCloudy").is_valid());
      CHECK(world.lookup("Patterns::Weather::Overcast").is_valid());
      CHECK(world.lookup("Patterns::Weather::Rain").is_valid());
      CHECK(world.lookup("Patterns::Weather::Thunderstorm").is_valid());
    }

    THEN("Each pattern has the WeatherPattern component") {
      for (const char *name :
           {"Patterns::Weather::Clear", "Patterns::Weather::PartlyCloudy",
            "Patterns::Weather::Overcast", "Patterns::Weather::Rain",
            "Patterns::Weather::Thunderstorm"}) {
        CHECK(world.lookup(name).has<WeatherPattern>());
      }
    }

    THEN("Each pattern is also an Effect entity") {
      for (const char *name :
           {"Patterns::Weather::Clear", "Patterns::Weather::PartlyCloudy",
            "Patterns::Weather::Overcast", "Patterns::Weather::Rain",
            "Patterns::Weather::Thunderstorm"}) {
        CHECK(world.lookup(name).has<Effect>());
      }
    }

    THEN("Clear has no failure-rate modifier child") {
      auto clear = world.lookup("Patterns::Weather::Clear");
      bool has_modifier = false;
      clear.children([&](flecs::entity child) {
        if (child.has<Modifier>()) {
          has_modifier = true;
        }
      });
      CHECK(!has_modifier);
    }

    THEN("Thunderstorm has a failure-rate modifier of +0.25") {
      auto storm = world.lookup("Patterns::Weather::Thunderstorm");
      double additive = 0.0;
      storm.children([&](flecs::entity child) {
        if (child.has<Modifier>()) {
          additive = child.get<Modifier>().additive;
        }
      });
      CHECK(additive == 0.25);
    }

    THEN("Clear has outgoing WeatherTransition pairs") {
      auto clear = world.lookup("Patterns::Weather::Clear");
      auto transitions = getWeatherTransitions(clear);
      CHECK(!transitions.empty());
    }
  }
}

SCENARIO("systemAdvanceWeather", "[weather]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();

  auto clear = world.lookup("Patterns::Weather::Clear");
  auto rain = world.lookup("Patterns::Weather::Rain");

  GIVEN("A site with Clear weather before the transition day") {
    auto site = world.entity().add<Site>();
    site.add<HasEffect>(clear);
    site.set<CurrentWeather>(clear, {.next_transition_day = 10});
    world.get_mut<Game>().day = 5;

    WHEN("The system runs") {
      systemAdvanceWeather(site);

      THEN("Weather remains Clear") {
        CHECK(site.target<CurrentWeather>() == clear);
      }

      THEN("next_transition_day is unchanged") {
        CHECK(site.get<CurrentWeather>(clear).next_transition_day == 10);
      }

      THEN("HasEffect still points to Clear") {
        CHECK(site.has<HasEffect>(clear));
      }
    }
  }

  GIVEN("A site with weather that always transitions to Rain (100% "
        "probability)") {
    auto alwaysRain = world.entity();
    alwaysRain.set<WeatherPattern>({.icon_glyph = "x", .description = "Test"});
    alwaysRain.add<Effect>();
    alwaysRain.set<WeatherTransition>(rain, {1.0});

    auto site = world.entity().add<Site>();
    site.add<HasEffect>(alwaysRain);
    site.set<CurrentWeather>(alwaysRain, {.next_transition_day = 1});
    world.get_mut<Game>().day = 5;

    WHEN("The system runs") {
      systemAdvanceWeather(site);

      THEN("Weather transitions to Rain") {
        CHECK(site.target<CurrentWeather>() == rain);
      }

      THEN("HasEffect is updated to Rain") {
        CHECK(site.has<HasEffect>(rain));
        CHECK(!site.has<HasEffect>(alwaysRain));
      }

      THEN("next_transition_day advances by the interval") {
        CHECK(site.get<CurrentWeather>(rain).next_transition_day ==
              5 + WEATHER_TRANSITION_INTERVAL);
      }
    }
  }

  GIVEN("A site with weather that never transitions (no transitions defined)") {
    auto staticWeather = world.entity();
    staticWeather.set<WeatherPattern>(
        {.icon_glyph = "x", .description = "Static"});
    staticWeather.add<Effect>();

    auto site = world.entity().add<Site>();
    site.add<HasEffect>(staticWeather);
    site.set<CurrentWeather>(staticWeather, {.next_transition_day = 1});
    world.get_mut<Game>().day = 10;

    WHEN("The system runs") {
      systemAdvanceWeather(site);

      THEN("Weather stays the same") {
        CHECK(site.target<CurrentWeather>() == staticWeather);
      }

      THEN("HasEffect is unchanged") {
        CHECK(site.has<HasEffect>(staticWeather));
      }

      THEN("next_transition_day advances by the interval") {
        CHECK(site.get<CurrentWeather>(staticWeather).next_transition_day ==
              10 + WEATHER_TRANSITION_INTERVAL);
      }
    }
  }

  GIVEN("Two sites with different weather") {
    auto alwaysRain = world.entity();
    alwaysRain.set<WeatherPattern>({.icon_glyph = "x", .description = "Test"});
    alwaysRain.add<Effect>();
    alwaysRain.set<WeatherTransition>(rain, {1.0});

    auto site1 = world.entity().add<Site>();
    site1.add<HasEffect>(alwaysRain);
    site1.set<CurrentWeather>(alwaysRain, {.next_transition_day = 1});

    auto site2 = world.entity().add<Site>();
    site2.add<HasEffect>(clear);
    site2.set<CurrentWeather>(clear, {.next_transition_day = 100});

    world.get_mut<Game>().day = 5;

    WHEN("Advance is called only on site1") {
      systemAdvanceWeather(site1);

      THEN("Site1 transitions to Rain") {
        CHECK(site1.target<CurrentWeather>() == rain);
      }

      THEN("Site2 is unaffected") {
        CHECK(site2.target<CurrentWeather>() == clear);
        CHECK(site2.has<HasEffect>(clear));
      }
    }
  }
}

SCENARIO("systemInitSiteWeather", "[weather]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();

  auto clear = world.lookup("Patterns::Weather::Clear");

  GIVEN("A site with no weather assigned") {
    auto site = world.entity().add<Site>();
    REQUIRE(!site.has<CurrentWeather>(flecs::Wildcard));

    WHEN("The init system runs") {
      systemInitSiteWeather(site, site.get<Site>());

      THEN("Site gets Clear weather") {
        CHECK(site.target<CurrentWeather>() == clear);
      }

      THEN("HasEffect points to Clear") { CHECK(site.has<HasEffect>(clear)); }

      THEN("next_transition_day is set to the transition interval") {
        CHECK(site.get<CurrentWeather>(clear).next_transition_day ==
              WEATHER_TRANSITION_INTERVAL);
      }
    }
  }
}
