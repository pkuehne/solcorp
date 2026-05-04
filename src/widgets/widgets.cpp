#include "widgets.h"
#include "imgui.h"

bool ActionButton(ButtonLabel label, ButtonTooltip tooltip,
                  const std::string &issue) {
  bool retval = false;
  ImGui::BeginDisabled(!issue.empty());
  if (ImGui::Button(label.text)) {
    retval = true;
  }
  if ((tooltip.text || !issue.empty()) && ImGui::BeginItemTooltip()) {
    if (tooltip.text)
      ImGui::Text("%s", tooltip.text);
    if (!issue.empty())
      ImGui::TextColored((ImVec4)ImColor::HSV(1.0, 1.0, 1.0), "%s",
                         issue.c_str());
    ImGui::EndTooltip();
  }
  ImGui::EndDisabled();
  return retval;
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
