#include "main_menu.h"
#include "components.h"
#include "imgui.h"
#include <flecs.h>

void systemDrawMainMenu(flecs::entity winE, MainMenuBar);

MainMenuModule::MainMenuModule(flecs::world &world) {
  flecs::entity GuiPhase = world.lookup("Phase.Gui");

  // Register components
  world.component<MainMenuBar>();

  // Register window
  world.entity("MainMenuBar").add<MainMenuBar>();

  // Register systems
  world.system<MainMenuBar>("Draw LaunchWindow")
      .kind(GuiPhase)
      .each(systemDrawMainMenu);
}

void systemDrawMainMenu(flecs::entity winE, MainMenuBar) {
  auto world = winE.world();
  GameResource *game = world.get_mut<GameResource>();

  if (ImGui::BeginMainMenuBar()) {
    ImGui::PushItemWidth(-FLT_MIN);
    ImGui::Text("Day: %3d", game->day);
    if (ImGui::Button(game->sim_speed.enabled() ? "||" : ">")) {
      game->sim_speed.enabled() ? game->sim_speed.disable()
                                : game->sim_speed.enable();
    }
    ImGui::PopItemWidth();
    ImGui::EndMainMenuBar();
  }
}
