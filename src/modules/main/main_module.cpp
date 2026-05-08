#include "main_module.h"
#include "SDL_keycode.h"
#include "imgui.h"
#include "modules/base/base.h"
#include "modules/engine/input.h"
#include "modules/lua/lua.h"
#include <flecs.h>
#include <modules/rocket/active_launches_window.h>
#include <modules/rocket/contracts_window.h>
#include <modules/rocket/launch_window.h>
#include <modules/simulation/celestial_browser.h>
#include <modules/simulation/developer_window.h>
#include <modules/simulation/simulation.h>

void systemDrawMainMenu(flecs::entity, const Simulation, const Game,
                        MainMenuBar);
void systemToggle(flecs::iter &, size_t, Simulation &, const KeyDown);

MainModule::MainModule(flecs::world &world) {

  // Register components
  world.component<MainMenuBar>();
  world.component<EffortRequired>()
      .member("remaining", &EffortRequired::remaining)
      .member("total", &EffortRequired::total);
  world.component<DurationRequired>()
      .member("remaining", &DurationRequired::remaining)
      .member("total", &DurationRequired::total);

  // Register lua bindings
  register_component_lua<EffortRequired>(
      world, "EffortRequired", [](LuaFieldBuilder<EffortRequired> &b) {
        b.field<&EffortRequired::remaining>("remaining")
            .field<&EffortRequired::total>("total");
      });
  register_component_lua<DurationRequired>(
      world, "DurationRequired", [](LuaFieldBuilder<DurationRequired> &b) {
        b.field<&DurationRequired::remaining>("remaining")
            .field<&DurationRequired::total>("total");
      });

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
