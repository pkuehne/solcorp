#include "toolbar.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <modules/rocket/active_launches_window.h>
#include <modules/rocket/contracts_window.h>

void systemDrawToolbar(flecs::entity winE, Toolbar) {
  auto world = winE.world();

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  constexpr ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoScrollbar |
                                             ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_MenuBar;
  float height = ImGui::GetFrameHeight();

  if (ImGui::BeginViewportSideBar("##Toolbar", viewport, ImGuiDir_Up, height,
                                  toolbar_flags)) {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::SmallButton("Launches")) {
        showActiveLaunchesWindow(world);
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Contracts")) {
        showContractsWindow(world);
      }
      ImGui::EndMenuBar();
    }
  }
  ImGui::End();
}
