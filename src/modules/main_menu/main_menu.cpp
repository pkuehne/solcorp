#include "main_menu.h"
#include "SDL_keycode.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/input.h"
#include <flecs.h>
#include <modules/rocket_launch/active_launches_window.h>
#include <modules/rocket_launch/contracts_window.h>
#include <modules/rocket_launch/launch_window.h>
#include <modules/simulation/celestial_browser.h>
#include <modules/simulation/developer_window.h>
#include <modules/simulation/simulation.h>

void systemDrawMainMenu(flecs::entity, const Simulation, const Game,
                        MainMenuBar);
void systemToggle(flecs::iter &, size_t, Simulation &, const KeyDown);

MainMenuModule::MainMenuModule(flecs::world &world) {

  // Register components
  world.component<MainMenuBar>();

  // Register window
  world.entity("MainMenuBar").add<MainMenuBar>();

  // Register Systems
  world.system<const Simulation, const Game, MainMenuBar>("Draw MainMenu")
      .kind(GuiPhase)
      .each(systemDrawMainMenu);
  world.system<Simulation, const KeyDown>("Toggle Play/Pause")
      .kind(ValidatePhase)
      .each(systemToggle);
}

void systemDrawMainMenu(flecs::entity winE, const Simulation sim,
                        const Game game, MainMenuBar) {
  auto world = winE.world();
  auto company = world.get<Company>();

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
    ImGui::Text(" $ %ld ", company.balance);
    if (ImGui::BeginMenu("Windows")) {
      if (ImGui::MenuItem("Celestial Browser")) {
        showCelestialBrowser(world);
      }
      if (ImGui::MenuItem("Launch Planner")) {
        showLaunchWindowAdd(world);
      }
      if (ImGui::MenuItem("Active Launches")) {
        showActiveLaunchesWindow(world);
      }
      if (ImGui::MenuItem("Contracts")) {
        showContractsWindow(world);
      }
      if (ImGui::MenuItem("Developer Window")) {
        showDeveloperWindow(world);
      }
      ImGui::EndMenu();
    }
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

void systemToggle(flecs::iter &it, size_t, Simulation &sim,
                  const KeyDown event) {
  if (event.key == SDLK_SPACE) {
    auto simTimer = it.world().timer(sim.speed.id());
    simTimer.get<flecs::Timer>().active ? simTimer.stop() : simTimer.start();
  }
}
