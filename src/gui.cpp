#include "gui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "components.h"
#include "gui/site_window.h"
#include "imgui.h"
#include "spdlog/spdlog.h"

/// @brief Creates and initialises the GUI system
/// @param world
void initialiseGUI(flecs::world &world) {
  spdlog::info("Creating GUI");

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  auto io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

  auto r = world.get_mut<Renderer>();
  ImGui_ImplSDL2_InitForSDLRenderer(r->window, r->renderer);
  ImGui_ImplSDLRenderer2_Init(r->renderer);
}

/// @brief System to update the UI from the current game state
/// @param it
/// @param game The game resource
/// @param r The renderer
void systemUpdateUI(flecs::iter &it, GuiResource *gui) {
  auto world = it.world();

  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  if (gui->show_demo_window) {
    ImGui::ShowDemoWindow();
  }
  gui->main_menu.draw(world);
  gui->site_window.draw(world);
  gui->launch_window.draw(world);

  ImGui::EndFrame();
  ImGui::Render();
}

/// @brief System to send render instructions for the UI
/// @param it
/// @param renderer The Render Component Singleton
void systemRenderUI(flecs::iter &, const Renderer *r) {
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), r->renderer);
}
