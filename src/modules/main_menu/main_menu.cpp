#include "main_menu.h"
#include "SDL_keycode.h"
#include "imgui.h"
#include "modules/gui/gui.h"
#include "modules/input/input.h"
#include "modules/phase/phase.h"
#include "modules/simulation/simulation.h"
#include <flecs.h>

void systemDrawMainMenu(flecs::entity, const Simulation, const Game,
                        MainMenuBar);
void systemToggle(flecs::iter &, size_t, Simulation &, const KeyDown);

MainMenuModule::MainMenuModule(flecs::world &world) {
  world.import <InputModule>();
  world.import <PhaseModule>();
  world.import <GuiModule>();

  // Register components
  world.component<MainMenuBar>();

  // Register window
  world.entity("MainMenuBar").add<MainMenuBar>();

  // Register Systems
  world.system<const Simulation, const Game, MainMenuBar>("Draw MainMenu")
      .term_at(0)
      .singleton()
      .term_at(1)
      .singleton()
      .kind(GuiPhase)
      .each(systemDrawMainMenu);
  world.system<Simulation, const KeyDown>("Toggle Play/Pause")
      .term_at(0)
      .singleton()
      .term_at(1)
      .singleton()
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
    ImGui::PopItemWidth();
    ImGui::EndMainMenuBar();
  }
}

void systemToggle(flecs::iter &, size_t, Simulation &sim, const KeyDown event) {
  if (event.key == SDLK_SPACE) {
    sim.speed.enabled() ? sim.speed.disable() : sim.speed.enable();
  }
}
