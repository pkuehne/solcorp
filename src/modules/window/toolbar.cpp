#include "toolbar.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "modules/rocket/launch_window.h"
#include "modules/simulation/celestial_browser.h"
#include <modules/rocket/active_launches_window.h>
#include <modules/rocket/contracts_window.h>
#include <modules/window/notification_window.h>

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
      if (ImGui::Button("\xef\x84\xb5")) { // fa-rocket f135
        showActiveLaunchesWindow(world);
      }
      ImGui::SetItemTooltip("View Active Launches");
      ImGui::SameLine();
      if (ImGui::Button("\xef\x95\xac")) { // fa-file-contract f56c
        showContractsWindow(world);
      }
      ImGui::SetItemTooltip("View Contracts");
      ImGui::SameLine();
      if (ImGui::Button("\xef\x95\xbc")) { // fa-earth-africa f57c
        showCelestialBrowser(world);
      }
      ImGui::SetItemTooltip("Celestial Browser");
      ImGui::SameLine();
      if (ImGui::Button("\xef\x89\xb1")) { // fa-calendar-plus f271
        showLaunchWindowAdd(world);
      }
      ImGui::SetItemTooltip("Plan a Launch");

      static const char *envelope = "\xef\x83\xa0"; // fa-envelope f0e0
      float envelope_width = ImGui::CalcTextSize(envelope).x +
                             ImGui::GetStyle().FramePadding.x * 2;
      ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - envelope_width);
      if (ImGui::Button(envelope)) {
        showNotificationWindow(world);
      }
      ImGui::SetItemTooltip("Notifications");
      ImGui::EndMenuBar();
    }
  }
  ImGui::End();
}
