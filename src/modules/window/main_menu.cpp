#include "main_menu.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/helpers.h"
#include <flecs.h>
#include <modules/simulation/simulation.h>

void systemDrawMainMenu(flecs::entity winE, const Simulation sim,
                        const Game game, MainMenuBar) {
  auto world = winE.world();
  const auto &company = world.get<Company>();

  if (company.balance < 0) {
    ImGui::OpenPopup("Game Over");
  }

  if (ImGui::BeginMainMenuBar()) {
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::Text("Day: %3d", game.day);
    auto simTimer = winE.world().timer(sim.speed.id());
    bool running = simTimer.get<flecs::Timer>().active;
    if (ImGui::Button(running ? "||" : ">")) {
      running ? simTimer.stop() : simTimer.start();
    }
    ImGui::Text(" %s ", company.name.c_str());
    ImGui::Text(" $%s ", format_locale(company.balance).c_str());

    ImGui::PopItemWidth();
    ImGui::EndMainMenuBar();
  }

  if (ImGui::BeginPopupModal("Game Over", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Your balance is negative. The corporation is bankrupt.");
    if (ImGui::Button("OK")) {
      ImGui::CloseCurrentPopup();
      world.quit();
    }
    ImGui::EndPopup();
  }
}
