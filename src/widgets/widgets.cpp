#include "widgets.h"
#include "imgui.h"

namespace Widgets {

namespace {
bool actionButtonImpl(bool small, ButtonLabel label, ButtonTooltip tooltip,
                      const std::string &issue) {
  ImGui::BeginDisabled(!issue.empty());
  bool retval = small ? ImGui::SmallButton(label.text) : ImGui::Button(label.text);
  if ((tooltip.text || !issue.empty()) && ImGui::BeginItemTooltip()) {
    if (tooltip.text) {
      ImGui::Text("%s", tooltip.text);
    }
    if (!issue.empty()) {
      ImGui::TextColored((ImVec4)ImColor::HSV(1.0, 1.0, 1.0), "%s",
                         issue.c_str());
    }
    ImGui::EndTooltip();
  }
  ImGui::EndDisabled();
  return retval;
}
} // namespace

bool ActionButton(ButtonLabel label, ButtonTooltip tooltip,
                  const std::string &issue) {
  return actionButtonImpl(false, label, tooltip, issue);
}

bool SmallActionButton(ButtonLabel label, ButtonTooltip tooltip,
                       const std::string &issue) {
  return actionButtonImpl(true, label, tooltip, issue);
}

void NotImplementedPopup() {
  // Always center this window when appearing
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal("Not Implemented", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("This feature has not yet been implemented!");

    ImGui::SetItemDefaultFocus();
    if (ImGui::Button("OK")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

} // namespace Widgets
