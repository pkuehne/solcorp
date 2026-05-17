#include "stat_widget.h"
#include "imgui.h"
#include <format>
#include <functional>

namespace Widgets {

void StatTooltip(const Stat *stat,
                 const std::function<std::string(double)> &fmt) {
  ImGui::Text("%s: %s", stat->display().c_str(), fmt(stat->value()).c_str());

  if (ImGui::BeginItemTooltip()) {
    ImGui::Text("%s", stat->description().c_str());
    ImGui::Separator();
    ImGui::Text("Base Value: %s", fmt(stat->base()).c_str());
    constexpr ImVec4 red = ImVec4(1.0, 0.0, 0.0, 1.0);
    constexpr ImVec4 green = ImVec4(0.0, 0.5, 0.0, 1.0);
    for (const auto &item : stat->modifiers()) {
      std::string modValue = "";
      ImVec4 colour;
      if (item.mod.additive > 0) {
        modValue = "+" + fmt(item.mod.additive);
        colour = stat->isHigherBetter() ? green : red;
      } else if (item.mod.additive < 0) {
        modValue = fmt(item.mod.additive);
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
      if (!modValue.empty()) {
        ImGui::TextColored(colour, "%s", modValue.c_str());
      }
    }
    ImGui::Separator();
    ImGui::Text("Final Value: %s", fmt(stat->value()).c_str());
    ImGui::EndTooltip();
  }
}

} // namespace Widgets
