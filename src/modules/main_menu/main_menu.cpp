#include "main_menu.h"
#include "SDL_keycode.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/input.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>
#include <modules/rocket_launch/launch_window.h>
#include <modules/simulation/celestial_browser.h>

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

  if (ImGui::BeginMainMenuBar()) {
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::Text("Day: %3d", game.day);
    if (ImGui::Button(sim.speed.enabled() ? "||" : ">")) {
      sim.speed.enabled() ? sim.speed.disable() : sim.speed.enable();
    }
    if (ImGui::BeginMenu("Windows")) {
      if (ImGui::MenuItem("Celestial Browser")) {
        showCelestialBrowser(world);
      }
      if (ImGui::MenuItem("Launch Planner")) {
        showLaunchWindowAdd(world);
      }
      ImGui::EndMenu();
    }
    ImGui::PopItemWidth();
    ImGui::EndMainMenuBar();
  }
}

void systemToggle(flecs::iter &, size_t, Simulation &sim, const KeyDown event) {
  if (event.key == SDLK_SPACE) {
    sim.speed.enabled() ? sim.speed.disable() : sim.speed.enable();
  }
}
