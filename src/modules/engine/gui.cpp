#include "gui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "imgui.h"
#include "modules/engine/engine.h"
#include "modules/engine/render.h"
#include "spdlog/spdlog.h"
#include <flecs.h>

void systemInitialiseGui(flecs::iter &iter);
void systemGuiNewFrame(flecs::iter &);
void systemGuiEndFrame(flecs::iter &);
void systemRenderGUI(const Renderer &);

void registerGui(flecs::world &world) {
  world.import <EngineModule>();

  // Register Systems
  world.system("Initiatlise GUI").kind(flecs::OnStart).run(systemInitialiseGui);
  world.system("New GUI Frame").kind(PreFramePhase).run(systemGuiNewFrame);
  world.system("End GUI Frame").kind(PreRenderPhase).run(systemGuiEndFrame);
  world.system<const Renderer>("Render GUI")
      .term_at(0)
      .singleton()
      .kind(RenderPhase)
      .each(systemRenderGUI);
}

/// @brief Creates and initialises the GUI system
/// @param world
void systemInitialiseGui(flecs::iter &iter) {
  spdlog::info("Creating GUI");

  auto world = iter.world();

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();

  ImVector<ImWchar> ranges;
  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(io.Fonts->GetGlyphRangesDefault()); // Add the default range
  builder.AddChar(0xf135); // Add a specific character
  builder.BuildRanges(&ranges);

  ImFontConfig config;
  io.Fonts->AddFontFromFileTTF("custom-font.ttf", 18.0f, &config, ranges.Data);
  io.Fonts->Build();

  auto r = world.get_mut<Renderer>();
  ImGui_ImplSDL2_InitForSDLRenderer(r->window, r->renderer);
  ImGui_ImplSDLRenderer2_Init(r->renderer);
  ImGui_ImplSDLRenderer2_CreateFontsTexture();
}

/// @brief Task that creates a new ImGui Frame
void systemGuiNewFrame(flecs::iter &) {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  // ImGui::ShowMetricsWindow();
}

/// @brief Task that finishes up ImGui Frame creation
void systemGuiEndFrame(flecs::iter &) {
  ImGui::EndFrame();
  ImGui::Render();
}

/// @brief System to send render instructions for the UI
/// @param it
/// @param renderer The Render Component Singleton
void systemRenderGUI(const Renderer &r) {
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), r.renderer);
}
