#include "weather_widget.h"
#include "imgui.h"
#include "modules/site/site.h"
#include "modules/site/weather.h"
#include "modules/stats/stats.h"
#include <cctype>

static std::string formatStatName(const std::string &kebab) {
  std::string result;
  result.reserve(kebab.size());
  bool nextUpper = true;
  for (char c : kebab) {
    if (c == '-') {
      result += ' ';
      nextUpper = true;
    } else if (nextUpper) {
      result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      nextUpper = false;
    } else {
      result += c;
    }
  }
  return result;
}

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
        if (mod.additive != 0.0) {
          ImColor colour = (mod.additive > 0) ? ImColor(220, 80, 80, 255)
                                              : ImColor(80, 200, 80, 255);
          ImGui::Text("%s:", formatStatName(mod.target_stat).c_str());
          ImGui::SameLine();
          ImGui::TextColored(colour, "%+.0f%%", mod.additive * 100.0);
        }
      });
    }

    ImGui::EndTooltip();
  }
}
