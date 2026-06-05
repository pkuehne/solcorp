#include "modules/site/weather.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

namespace {
flecs::world makeWorld() {
  flecs::world world;
  world.component<WeatherPattern>();
  world.component<WeatherTransition>();
  world.component<CurrentWeather>().add(flecs::Exclusive);
  return world;
}
} // namespace

SCENARIO("selectNextWeather", "[weather]") {
  auto world = makeWorld();
  auto clear = world.entity("Clear").set<WeatherPattern>(
      {.icon_glyph = "sun", .description = "Clear skies"});
  auto overcast = world.entity("Overcast")
                      .set<WeatherPattern>(
                          {.icon_glyph = "cloud", .description = "Overcast"});
  auto rain = world.entity("Rain").set<WeatherPattern>(
      {.icon_glyph = "rain", .description = "Rain"});

  GIVEN("No transitions defined") {
    WHEN("Any roll is made") {
      auto result = selectNextWeather(clear, {}, 0.5);

      THEN("Weather stays the same") { CHECK(result == clear); }
    }
  }

  GIVEN("A single transition with 30% chance") {
    std::vector<WeatherTransitionData> transitions = {{overcast, 0.30}};

    WHEN("Roll is strictly below threshold") {
      auto result = selectNextWeather(clear, transitions, 0.29);

      THEN("Transitions to target") { CHECK(result == overcast); }
    }

    WHEN("Roll equals the threshold exactly") {
      auto result = selectNextWeather(clear, transitions, 0.30);

      THEN("Stays the same (strict less-than boundary)") {
        CHECK(result == clear);
      }
    }

    WHEN("Roll exceeds the threshold") {
      auto result = selectNextWeather(clear, transitions, 0.90);

      THEN("Stays the same") { CHECK(result == clear); }
    }
  }

  GIVEN("Two transitions totalling 35%") {
    std::vector<WeatherTransitionData> transitions = {{overcast, 0.25},
                                                      {rain, 0.10}};

    WHEN("Roll falls in the first bucket (0–0.25)") {
      auto result = selectNextWeather(clear, transitions, 0.10);

      THEN("Transitions to overcast") { CHECK(result == overcast); }
    }

    WHEN("Roll falls in the second bucket (0.25–0.35)") {
      auto result = selectNextWeather(clear, transitions, 0.30);

      THEN("Transitions to rain") { CHECK(result == rain); }
    }

    WHEN("Roll exceeds all transition probabilities") {
      auto result = selectNextWeather(clear, transitions, 0.90);

      THEN("Weather stays the same") { CHECK(result == clear); }
    }

    WHEN("Roll is 0.0") {
      auto result = selectNextWeather(clear, transitions, 0.0);

      THEN("Falls into first bucket") { CHECK(result == overcast); }
    }
  }
}

SCENARIO("getWeatherTransitions", "[weather]") {
  auto world = makeWorld();
  auto clear = world.entity("Clear").set<WeatherPattern>(
      {.icon_glyph = "sun", .description = "Clear skies"});
  auto overcast = world.entity("Overcast")
                      .set<WeatherPattern>(
                          {.icon_glyph = "cloud", .description = "Overcast"});
  auto rain = world.entity("Rain").set<WeatherPattern>(
      {.icon_glyph = "rain", .description = "Rain"});

  GIVEN("A pattern with no transitions") {
    WHEN("Transitions are queried") {
      auto result = getWeatherTransitions(clear);

      THEN("Empty list is returned") { CHECK(result.empty()); }
    }
  }

  GIVEN("A pattern with two transitions") {
    clear.set<WeatherTransition>(overcast, {0.25});
    clear.set<WeatherTransition>(rain, {0.10});

    WHEN("Transitions are queried") {
      auto result = getWeatherTransitions(clear);

      THEN("Both transitions are returned") { CHECK(result.size() == 2); }

      THEN("Probabilities are preserved") {
        double total = 0.0;
        for (const auto &t : result) {
          total += t.probability;
        }
        CHECK(total == 0.35);
      }

      THEN("Targets are correct") {
        bool has_overcast = false;
        bool has_rain = false;
        for (const auto &t : result) {
          if (t.target == overcast) {
            has_overcast = true;
            CHECK(t.probability == 0.25);
          }
          if (t.target == rain) {
            has_rain = true;
            CHECK(t.probability == 0.10);
          }
        }
        CHECK(has_overcast);
        CHECK(has_rain);
      }
    }
  }

  GIVEN("Querying a pattern does not return transitions of other patterns") {
    clear.set<WeatherTransition>(overcast, {0.25});
    rain.set<WeatherTransition>(overcast, {0.40});

    WHEN("Clear's transitions are queried") {
      auto result = getWeatherTransitions(clear);

      THEN("Only clear's transition is returned") {
        REQUIRE(result.size() == 1);
        CHECK(result[0].target == overcast);
        CHECK(result[0].probability == 0.25);
      }
    }
  }
}
