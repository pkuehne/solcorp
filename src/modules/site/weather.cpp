#include "weather.h"
#include "modules/base/base.h"
#include "modules/engine/helpers.h"
#include "modules/stats/stats.h"
#include "site.h"
#include <spdlog/spdlog.h>

std::vector<WeatherTransitionData>
getWeatherTransitions(flecs::entity patternE) {
  std::vector<WeatherTransitionData> result;
  patternE.each<WeatherTransition>([&](flecs::entity target) {
    const auto &t = patternE.get<WeatherTransition>(target);
    result.push_back({target, t.probability});
  });
  return result;
}

flecs::entity
selectNextWeather(flecs::entity current,
                  const std::vector<WeatherTransitionData> &transitions,
                  double roll) {
  double cumulative = 0.0;
  for (const auto &t : transitions) {
    cumulative += t.probability;
    if (roll < cumulative) {
      return t.target;
    }
  }
  return current;
}

void systemAdvanceWeather(flecs::entity siteE) {
  auto current = siteE.target<CurrentWeather>();
  if (!current.is_valid()) {
    return;
  }
  const auto clock = siteE.get<CurrentWeather>(current);
  uint32_t today = siteE.world().get<Game>().day;
  if (today < clock.next_transition_day) {
    return;
  }
  auto transitions = getWeatherTransitions(current);
  auto next = selectNextWeather(current, transitions, random_double());
  if (next != current) {
    siteE.remove<HasEffect>(current);
    siteE.add<HasEffect>(next);
    spdlog::info("Site {} weather: {} -> {}", siteE.name().c_str(),
                 current.name().c_str(), next.name().c_str());
  }
  siteE.set<CurrentWeather>(next, {today + WEATHER_TRANSITION_INTERVAL});
}

void systemInitSiteWeather(flecs::entity siteE, const Site &) {
  auto clear = siteE.world().lookup("Patterns::Weather::Clear");
  if (!clear.is_valid()) {
    return;
  }
  siteE.add<HasEffect>(clear);
  siteE.set<CurrentWeather>(
      clear, {.next_transition_day = WEATHER_TRANSITION_INTERVAL});
}

void initWeather(flecs::world &world) {
  world.component<WeatherPattern>();
  world.component<WeatherTransition>();
  world.component<CurrentWeather>().add(flecs::Exclusive);

  // Escape module scope so all weather entities live at root and are
  // lookup-able by short path (Patterns::Weather::Clear, etc.)
  auto prevScope = world.set_scope(0);

  auto patternsNode = world.entity("Patterns");
  auto weatherNode = world.entity("Weather").child_of(patternsNode);

  auto makePattern = [&](const char *name, const char *glyph, const char *desc,
                         double failureAdd) {
    auto pattern =
        world.entity(name)
            .child_of(weatherNode)
            .set<WeatherPattern>({.icon_glyph = glyph, .description = desc})
            .add<Effect>();
    if (failureAdd != 0.0) {
      world.entity().child_of(pattern).set<Modifier>(
          {.target_stat = "failure-rate", .additive = failureAdd});
    }
    return pattern;
  };

  auto clear = makePattern("Clear", "\xef\x86\x85", "Clear skies", 0.00);
  auto partly =
      makePattern("PartlyCloudy", "\xef\x9b\x84", "Partly cloudy", 0.02);
  auto overcast = makePattern("Overcast", "\xef\x83\x82", "Overcast", 0.05);
  auto rain = makePattern("Rain", "\xef\x9c\xbd", "Rain", 0.12);
  auto thunderstorm =
      makePattern("Thunderstorm", "\xef\x9d\xac", "Thunderstorm", 0.25);

  clear.set<WeatherTransition>(partly, {0.25});
  clear.set<WeatherTransition>(overcast, {0.10});

  partly.set<WeatherTransition>(clear, {0.20});
  partly.set<WeatherTransition>(overcast, {0.30});
  partly.set<WeatherTransition>(rain, {0.10});

  overcast.set<WeatherTransition>(partly, {0.15});
  overcast.set<WeatherTransition>(rain, {0.35});
  overcast.set<WeatherTransition>(thunderstorm, {0.05});

  rain.set<WeatherTransition>(partly, {0.05});
  rain.set<WeatherTransition>(overcast, {0.25});
  rain.set<WeatherTransition>(thunderstorm, {0.20});

  thunderstorm.set<WeatherTransition>(overcast, {0.20});
  thunderstorm.set<WeatherTransition>(rain, {0.40});

  world.set_scope(prevScope);

  auto sim = world.get<Simulation>();

  world.system<>("Advance Weather")
      .with<CurrentWeather>(flecs::Wildcard)
      .kind(UpdatePhase)
      .tick_source(sim.speed)
      .each([](flecs::entity siteE) { systemAdvanceWeather(siteE); });

  world.system<const Site>("Init Site Weather")
      .without<CurrentWeather>(flecs::Wildcard)
      .kind(UpdatePhase)
      .each(systemInitSiteWeather);
}
