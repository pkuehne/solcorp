#include "weather.h"

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
