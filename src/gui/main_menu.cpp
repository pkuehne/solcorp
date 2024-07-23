#include "main_menu.h"
#include "components.h"
#include "imgui.h"

void MainMenu::draw(flecs::world &world) {
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
