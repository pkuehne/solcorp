#include "stat_widget.h"
#include "imgui.h"
#include "modules/engine/helpers.h"
#include <format>

namespace Widgets {

namespace {

std::string formatModifierValue(double value, Stat::Format format) {
  switch (format) {
  case Stat::Format::Currency:
    return "$" + format_locale(value);
  case Stat::Format::Percentage:
    return std::format("{:.0f}%", value * 100);
  case Stat::Format::Number:
  default:
    return std::format("{:.0f}", value);
  }
}

ImVec4 modifierColour(const Modifier &mod, const Stat *stat,
                      ModifierDisplayOptions options) {
  constexpr ImVec4 red = ImVec4(1.0, 0.0, 0.0, 1.0);
  constexpr ImVec4 green = ImVec4(0.0, 0.5, 0.0, 1.0);
  constexpr ImVec4 neutral = ImVec4(0.7, 0.7, 0.7, 1.0);
  const bool higherIsBetter =
      stat ? stat->isHigherBetter() : options.fallback_higher_is_better;

  if (mod.additive > 0.0 || mod.multiplicative > 1.0) {
    return higherIsBetter ? green : red;
  }
  if (mod.additive < 0.0 || mod.multiplicative < 1.0) {
    return higherIsBetter ? red : green;
  }
  return neutral;
}

} // namespace

std::string ModifierValueText(const Modifier &mod, const Stat *stat,
                              ModifierDisplayOptions options) {
  if (mod.additive > 0.0) {
    return "+" + (stat ? stat->format(mod.additive)
                       : formatModifierValue(mod.additive,
                                             options.fallback_format));
  }
  if (mod.additive < 0.0) {
    return stat ? stat->format(mod.additive)
                : formatModifierValue(mod.additive, options.fallback_format);
  }
  if (mod.multiplicative > 1.0) {
    return std::format("+{:.0f}%", (mod.multiplicative - 1.0) * 100);
  }
  if (mod.multiplicative < 1.0) {
    return std::format("{:.0f}%", (mod.multiplicative - 1.0) * 100);
  }
  return {};
}

bool ModifierLine(const char *label, const Modifier &mod, const Stat *stat,
                  ModifierDisplayOptions options) {
  const auto modValue = ModifierValueText(mod, stat, options);
  if (modValue.empty()) {
    return false;
  }

  ImGui::Text("%s:", label);
  ImGui::SameLine();
  ImGui::TextColored(modifierColour(mod, stat, options), "%s",
                     modValue.c_str());
  return true;
}

void StatTooltip(const Stat *stat) {
  ImGui::Text("%s: %s", stat->display().c_str(),
              stat->format(stat->value()).c_str());

  if (ImGui::BeginItemTooltip()) {
    ImGui::Text("%s", stat->description().c_str());
    ImGui::Separator();
    ImGui::Text("Base Value: %s", stat->format(stat->base()).c_str());
    for (const auto &item : stat->modifiers()) {
      ModifierLine(item.effectName.c_str(), item.mod, stat);
    }
    ImGui::Separator();
    ImGui::Text("Final Value: %s", stat->format(stat->value()).c_str());
    ImGui::EndTooltip();
  }
}

} // namespace Widgets
