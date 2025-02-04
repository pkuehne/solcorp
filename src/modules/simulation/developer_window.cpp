#include "developer_window.h"
#include "imgui.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void showDeveloperWindow(flecs::world &world) {
  spdlog::info("Showing Developer Window");
  auto win = DeveloperWindow();
  world.entity("DeveloperWindow").set<DeveloperWindow>(win);
}

void hideDeveloperWindow(flecs::world &world) {
  spdlog::debug("Hiding DeveloperWindow");
  auto entity = world.lookup("DeveloperWindow");
  entity.destruct();
}

void systemDrawDeveloperWindow(flecs::entity winE, DeveloperWindow &win) {
  auto world = winE.world();
  if (!win.open) {
    hideDeveloperWindow(world);
    return;
  }
  auto options = world.get_mut<Developer>();

  ImGui::Begin("Developer Tools", &win.open);
  ImGui::Checkbox("Show Metrics Window", &options->show_metrics_window);
  ImGui::End();

  if (options->show_metrics_window) {
    ImGui::ShowMetricsWindow();
  }
}
