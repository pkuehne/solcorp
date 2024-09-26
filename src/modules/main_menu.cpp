#include "main_menu.h"
#include "imgui.h"
#include "modules/gui.h"
#include "modules/phase.h"
#include "modules/simulation.h"
#include <flecs.h>

void systemDrawMainMenu(flecs::entity winE, const Simulation, const Game,
                        MainMenuBar);

MainMenuModule::MainMenuModule(flecs::world &world) {
  world.import <phase>();
  world.import <GuiModule>();

  flecs::entity GuiPhase = world.lookup("phase.Gui");

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
