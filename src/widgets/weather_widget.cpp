#include "weather_widget.h"
#include "imgui.h"
#include "modules/site/site.h"
#include "modules/site/weather.h"
#include "modules/stats/stats.h"
#include "widgets/stat_widget.h"

static Stat *findAffectedStat(flecs::entity root, const std::string &id) {
  if (auto *stat = findStat(root, id)) {
    return stat;
  }

  Stat *result = nullptr;
  root.children([&](flecs::entity child) {
    if (result) {
      return;
    }
    result = findAffectedStat(child, id);
  });
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
        auto *stat = findAffectedStat(siteE, mod.target_stat);
        const auto &label = stat ? stat->display() : mod.target_stat;
        Widgets::ModifierLine(
            label.c_str(), mod, stat,
            {.fallback_format = Stat::Format::Percentage,
             .fallback_higher_is_better = false});
      });
    }

    ImGui::EndTooltip();
  }
}
