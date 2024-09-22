#include "widgets.h"
#include "imgui.h"

bool ActionButton(const char *label, const char *tooltip,
                  const std::string &issue) {
  bool retval = false;
  ImGui::BeginDisabled(!issue.empty());
  if (ImGui::Button(label)) {
    retval = true;
  }
  if ((tooltip || !issue.empty()) && ImGui::BeginItemTooltip()) {
    if (tooltip)
      ImGui::Text("%s", tooltip);
    if (!issue.empty())
      ImGui::TextColored((ImVec4)ImColor::HSV(1.0, 1.0, 1.0), "%s",
                         issue.c_str());
    ImGui::EndTooltip();
  }
  ImGui::EndDisabled();
  return retval;
}
