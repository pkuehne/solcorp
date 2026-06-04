#pragma once

#include <cstdint>
#include <flecs.h>
#include <string>
#include <vector>

struct Site;

struct WeatherPattern {
  std::string icon_glyph;
  std::string description;
};

struct WeatherTransition {
  double probability;
};
// Used as pair: source.set<WeatherTransition>(target, {probability})

struct CurrentWeather {
  uint32_t next_transition_day = 7;
};
// Exclusive pair: site.set<CurrentWeather>(patternE, {next_transition_day})
// site.target<CurrentWeather>() retrieves the active pattern
// site.get_mut<CurrentWeather>(patternE) accesses the clock

struct WeatherTransitionData {
  flecs::entity target;
  double probability;
};

constexpr uint32_t WEATHER_TRANSITION_INTERVAL = 7;

std::vector<WeatherTransitionData>
getWeatherTransitions(flecs::entity patternE);

// roll must be in [0.0, 1.0). Returns current if no transition fires.
flecs::entity
selectNextWeather(flecs::entity current,
                  const std::vector<WeatherTransitionData> &transitions,
                  double roll);

void systemAdvanceWeather(flecs::entity siteE);
void systemInitSiteWeather(flecs::entity siteE, const Site &);
void initWeather(flecs::world &world);
