#include "developer_window.h"
#include "imgui.h"
#include "modules/engine/gui.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

void drawCheatsTab(flecs::world &);

void showDeveloperWindow(flecs::world &world) {
  showWindow(world, "Developer Window");
}

void child_tree(flecs::entity e) {
  ImGui::PushID(std::to_string(e.id()).c_str());
  if (ImGui::TreeNode("", "%s", e.name().c_str())) {
    e.children([](flecs::entity e) { child_tree(e); });
    ImGui::TreePop();
  }
  ImGui::PopID();
};

void drawDeveloperWindow(flecs::entity winE) {
  auto &state = winE.get_mut<DeveloperWindow>();
  auto world = winE.world();
  ImGui::Checkbox("Show Metrics Window", &state.show_metrics_window);

  if (ImGui::BeginTabBar("Tools")) {
    if (ImGui::BeginTabItem("Cheats")) {
      drawCheatsTab(world);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Entity Tree")) {
      world.children([](flecs::entity e) { child_tree(e); });
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  if (state.show_metrics_window) {
    ImGui::ShowMetricsWindow();
  }
}

void drawCheatsTab(flecs::world &world) {
  if (ImGui::Button("Add $10M to balance")) {
    world.get_mut<Company>().balance += 10'000'000;
  }
}