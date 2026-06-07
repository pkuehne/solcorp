#include "stat_widget.h"
#include "imgui.h"
#include <format>

namespace Widgets {

namespace {

ImVec4 modifierColour(const Modifier &mod, const StatDef &definition) {
  constexpr ImVec4 red = ImVec4(1.0, 0.0, 0.0, 1.0);
  constexpr ImVec4 green = ImVec4(0.0, 0.5, 0.0, 1.0);
  constexpr ImVec4 neutral = ImVec4(0.7, 0.7, 0.7, 1.0);

  if (mod.additive > 0.0 || mod.multiplicative > 1.0) {
    return definition.higher_is_better ? green : red;
  }
  if (mod.additive < 0.0 || mod.multiplicative < 1.0) {
    return definition.higher_is_better ? red : green;
  }
  return neutral;
}

} // namespace

std::string ModifierValueText(const Modifier &mod, const StatDef &definition) {
  if (mod.additive > 0.0) {
    return "+" + definition.formatValue(mod.additive);
  }
  if (mod.additive < 0.0) {
    return definition.formatValue(mod.additive);
  }
  if (mod.multiplicative > 1.0) {
    return std::format("+{:.0f}%", (mod.multiplicative - 1.0) * 100);
  }
  if (mod.multiplicative < 1.0) {
    return std::format("{:.0f}%", (mod.multiplicative - 1.0) * 100);
  }
  return {};
}

bool ModifierLine(const char *label, const Modifier &mod,
                  const StatDef &definition) {
  const auto modValue = ModifierValueText(mod, definition);
  if (modValue.empty()) {
    return false;
  }

  ImGui::Text("%s:", label);
  ImGui::SameLine();
  ImGui::TextColored(modifierColour(mod, definition), "%s", modValue.c_str());
  return true;
}

void StatTooltip(flecs::world &world, const Stat *stat) {
  const auto definition = statDef(world, stat->id());
  ImGui::Text("%s: %s", definition.display.c_str(),
              definition.formatValue(stat->value()).c_str());

  if (ImGui::BeginItemTooltip()) {
    ImGui::Text("%s", definition.description.c_str());
    ImGui::Separator();
    ImGui::Text("Base Value: %s", definition.formatValue(stat->base()).c_str());
    for (const auto &item : stat->modifiers()) {
      ModifierLine(item.effectName.c_str(), item.mod, definition);
    }
    ImGui::Separator();
    ImGui::Text("Final Value: %s",
                definition.formatValue(stat->value()).c_str());
    ImGui::EndTooltip();
  }
}

} // namespace Widgets
