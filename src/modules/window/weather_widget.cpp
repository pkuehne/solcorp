#include "weather_widget.h"
#include "imgui.h"
#include "modules/site/site.h"
#include "modules/site/weather.h"
#include "modules/stats/stats.h"
#include <format>

void drawWeatherToolbarItem(flecs::world &world) {
  auto siteE =
      world.query_builder<const Site>().with<CurrentSite>().build().first();

  if (!siteE.is_valid()) {
    ImGui::TextDisabled("—");
    return;
  }

  auto currentPattern = siteE.target<CurrentWeather>();
  if (!currentPattern.is_valid()) {
    ImGui::TextDisabled("—");
    return;
  }

  const auto &wp = currentPattern.get<WeatherPattern>();

  if (ImGui::Button(wp.icon_glyph.c_str())) {
    // reserved for future detail pop-up
  }

  double modifier = 0.0;
  currentPattern.children([&](flecs::entity child) {
    if (child.has<Modifier>()) {
      modifier += child.get<Modifier>().additive;
    }
  });

  std::string tooltip = wp.description;
  if (modifier != 0.0) {
    tooltip += std::format(" — {:+.0f}% launch failure risk", modifier * 100.0);
  }
  ImGui::SetItemTooltip("%s", tooltip.c_str());
}
