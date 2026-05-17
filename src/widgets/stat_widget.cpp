#include "stat_widget.h"
#include "imgui.h"
#include <format>

namespace Widgets {

void StatTooltip(const Stat *stat) {
  ImGui::Text("%s: %.0f", stat->display().c_str(), stat->value());

  if (ImGui::BeginItemTooltip()) {
    ImGui::Text("%s", stat->description().c_str());
    ImGui::Separator();
    ImGui::Text("Base Value: %.0f", stat->base());
    constexpr ImVec4 red = ImVec4(1.0, 0.0, 0.0, 1.0);
    constexpr ImVec4 green = ImVec4(0.0, 0.5, 0.0, 1.0);
    for (const auto &item : stat->modifiers()) {
      std::string modValue = "";
      ImVec4 colour;
      if (item.mod.additive > 0) {
        modValue = std::format("+{:.0f}", item.mod.additive);
        colour = stat->isHigherBetter() ? green : red;
      } else if (item.mod.additive < 0) {
        modValue = std::format("{:.0f}", item.mod.additive);
        colour = stat->isHigherBetter() ? red : green;
      }
      if (item.mod.multiplicative > 1) {
        modValue = std::format("+{:.0f}%", (item.mod.multiplicative - 1) * 100);
        colour = stat->isHigherBetter() ? green : red;
      } else if (item.mod.multiplicative < 1) {
        modValue = std::format("{:.0f}%", (item.mod.multiplicative - 1) * 100);
        colour = stat->isHigherBetter() ? red : green;
      }
      ImGui::Text("%s:", item.effectName.c_str());
      ImGui::SameLine();
      ImGui::TextColored(colour, "%s", modValue.c_str());
    }
    ImGui::Separator();
    ImGui::Text("Final Value: %.0f", stat->value());
    ImGui::EndTooltip();
  }
}

} // namespace Widgets
