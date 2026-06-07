#include "weather_widget.h"
#include "imgui.h"
#include "modules/site/site.h"
#include "modules/site/weather.h"
#include "modules/stats/stats.h"
#include "widgets/stat_widget.h"

void drawWeatherToolbarItem(flecs::world &world) {
  static const char *placeholder = "\xef\x86\x85"; // fa-sun

  auto siteE =
      world.query_builder<const Site>().with<CurrentSite>().build().first();

  flecs::entity currentPattern;
  if (siteE.is_valid()) {
    currentPattern = siteE.target<CurrentWeather>();
  }

  const char *icon =
      (currentPattern.is_valid())
          ? currentPattern.get<WeatherPattern>().icon_glyph.c_str()
          : placeholder;
  ImGui::TextUnformatted(icon);

  if (!currentPattern.is_valid()) {
    return;
  }

  if (ImGui::BeginItemTooltip()) {
    const auto &wp = currentPattern.get<WeatherPattern>();
    ImGui::TextUnformatted(wp.description.c_str());

    bool hasModifiers = false;
    currentPattern.children([&](flecs::entity child) {
      if (child.has<Modifier>()) {
        hasModifiers = true;
      }
    });

    if (hasModifiers) {
      ImGui::Separator();
      currentPattern.children([&](flecs::entity child) {
        if (!child.has<Modifier>()) {
          return;
        }
        const auto &mod = child.get<Modifier>();
        const auto definition = statDef(world, mod.target_stat);
        Widgets::ModifierLine(definition.display.c_str(), mod, definition);
      });
    }

    ImGui::EndTooltip();
  }
}
