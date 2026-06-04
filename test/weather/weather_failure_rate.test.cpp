#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/site/weather.h"
#include "modules/stats/stats.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("Weather failure-rate modifier propagates to rockets", "[weather]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  auto clear = world.lookup("Patterns::Weather::Clear");
  auto rain = world.lookup("Patterns::Weather::Rain");
  auto thunderstorm = world.lookup("Patterns::Weather::Thunderstorm");

  GIVEN("A rocket stored inside a site with Clear weather") {
    auto site = world.entity().add<Site>();
    site.add<HasEffect>(clear);
    site.set<CurrentWeather>(clear, {.next_transition_day = 100});

    auto building = world.entity().child_of(site);
    auto facility = world.entity().child_of(building);
    auto rocket = world.entity().add<Rocket>().child_of(facility);

    WHEN("statsApplyModifiers is called on the rocket") {
      auto &r = rocket.get_mut<Rocket>();
      statsApplyModifiers(rocket, &r.failure_rate);

      THEN("Failure rate equals the base value (Clear adds no modifier)") {
        CHECK(r.failure_rate.value() == r.failure_rate.base());
      }
    }
  }

  GIVEN("A rocket stored inside a site with Rain weather") {
    auto site = world.entity().add<Site>();
    site.add<HasEffect>(rain);
    site.set<CurrentWeather>(rain, {.next_transition_day = 100});

    auto building = world.entity().child_of(site);
    auto facility = world.entity().child_of(building);
    auto rocket = world.entity().add<Rocket>().child_of(facility);

    WHEN("statsApplyModifiers is called on the rocket") {
      auto &r = rocket.get_mut<Rocket>();
      statsApplyModifiers(rocket, &r.failure_rate);

      THEN("Failure rate is higher than base by +0.12") {
        CHECK(r.failure_rate.value() == r.failure_rate.base() + 0.12);
      }
    }
  }

  GIVEN("A rocket stored inside a site with Thunderstorm weather") {
    auto site = world.entity().add<Site>();
    site.add<HasEffect>(thunderstorm);
    site.set<CurrentWeather>(thunderstorm, {.next_transition_day = 100});

    auto building = world.entity().child_of(site);
    auto facility = world.entity().child_of(building);
    auto rocket = world.entity().add<Rocket>().child_of(facility);

    WHEN("statsApplyModifiers is called on the rocket") {
      auto &r = rocket.get_mut<Rocket>();
      statsApplyModifiers(rocket, &r.failure_rate);

      THEN("Failure rate is base + 0.25") {
        CHECK(r.failure_rate.value() == r.failure_rate.base() + 0.25);
      }

      THEN("Failure rate is significantly higher than in Clear weather") {
        double clearRate = r.failure_rate.base();
        double stormRate = r.failure_rate.value();
        CHECK(stormRate > clearRate);
      }
    }
  }

  GIVEN("Two rockets at different sites with different weather") {
    auto site1 = world.entity().add<Site>();
    site1.add<HasEffect>(clear);
    site1.set<CurrentWeather>(clear, {.next_transition_day = 100});

    auto site2 = world.entity().add<Site>();
    site2.add<HasEffect>(thunderstorm);
    site2.set<CurrentWeather>(thunderstorm, {.next_transition_day = 100});

    auto rocket1 =
        world.entity().add<Rocket>().child_of(world.entity().child_of(site1));
    auto rocket2 =
        world.entity().add<Rocket>().child_of(world.entity().child_of(site2));

    WHEN("statsApplyModifiers is called on both rockets") {
      auto &r1 = rocket1.get_mut<Rocket>();
      auto &r2 = rocket2.get_mut<Rocket>();
      statsApplyModifiers(rocket1, &r1.failure_rate);
      statsApplyModifiers(rocket2, &r2.failure_rate);

      THEN("Rockets at different sites have different failure rates") {
        CHECK(r1.failure_rate.value() < r2.failure_rate.value());
      }

      THEN("Site1 rocket has base failure rate") {
        CHECK(r1.failure_rate.value() == r1.failure_rate.base());
      }

      THEN("Site2 rocket has base + 0.25 failure rate") {
        CHECK(r2.failure_rate.value() == r2.failure_rate.base() + 0.25);
      }
    }
  }

  GIVEN("A rocket NOT inside a site") {
    auto rocket = world.entity().add<Rocket>();

    WHEN("statsApplyModifiers is called") {
      auto &r = rocket.get_mut<Rocket>();
      statsApplyModifiers(rocket, &r.failure_rate);

      THEN(
          "Failure rate is the base value (no site ancestor to find weather)") {
        CHECK(r.failure_rate.value() == r.failure_rate.base());
      }
    }
  }
}
