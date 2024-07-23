#include "launch_window.h"
#include "imgui.h"
#include "spdlog/spdlog.h"

void loadData() {
  //
}

void LaunchWindow::draw(flecs::world &) {
  if (!visible)
    return;

  if (launchPlanEntity == flecs::entity() || !launchPlanEntity.is_alive()) {
    spdlog::error("Opened LaunchWindow on non-existant launch plan");
    visible = false;
    return;
  }
  this->loadData();
  ImGui::Begin("Site");
  ImGui::End();
}
