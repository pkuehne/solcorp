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

void child_tree(flecs::entity e) {
  ImGui::PushID(e.id());
  if (ImGui::TreeNode("", "%s", e.name().c_str())) {
    e.children(child_tree);
    ImGui::TreePop();
  }
  ImGui::PopID();
};

void systemDrawDeveloperWindow(flecs::entity winE, DeveloperWindow &win) {
  auto world = winE.world();
  if (!win.open) {
    hideDeveloperWindow(world);
    return;
  }
  auto options = world.get_mut<Developer>();

  ImGui::Begin("Developer Tools", &win.open);
  ImGui::Checkbox("Show Metrics Window", &options->show_metrics_window);
  world.children(child_tree);
  ImGui::End();

  if (options->show_metrics_window) {
    ImGui::ShowMetricsWindow();
  }
}
