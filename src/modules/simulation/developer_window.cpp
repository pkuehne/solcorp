#include "developer_window.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void showDeveloperWindow(flecs::world &world) {
  showWindow(world, "Developer Window");
}

void child_tree(flecs::entity e) {
  ImGui::PushID(e.id());
  if (ImGui::TreeNode("", "%s", e.name().c_str())) {
    e.children(child_tree);
    ImGui::TreePop();
  }
  ImGui::PopID();
};

void drawDeveloperWindow(flecs::entity winE) {
  auto &state = winE.get_mut<DeveloperWindow>();
  auto world = winE.world();
  ImGui::Checkbox("Show Metrics Window", &state.show_metrics_window);

  if (ImGui::BeginTabBar("Tools")) {
    if (ImGui::BeginTabItem("Entity Tree")) {
      world.children(child_tree);
    }
  }

  if (state.show_metrics_window) {
    ImGui::ShowMetricsWindow();
  }
}
