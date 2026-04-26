#include "gui.h"
#include "SDL_video.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "imgui.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/engine/render.h"
#include "spdlog/spdlog.h"
#include <flecs.h>

void systemInitialiseGui(flecs::iter &iter);
void systemGuiNewFrame(flecs::iter &);
void systemGuiEndFrame(flecs::iter &);
void systemRenderGUI(const Renderer &);
void systemRenderWindow(flecs::entity winE, Window &win);

void registerGui(flecs::world &world) {
  world.import<EngineModule>();

  // Register Systems
  world.system("Initialise GUI").kind(flecs::OnStart).run(systemInitialiseGui);
  world.system("New GUI Frame").kind(PreFramePhase).run(systemGuiNewFrame);
  world.system("End GUI Frame").kind(PreRenderPhase).run(systemGuiEndFrame);
  world.system<const Renderer>("Render GUI")
      .kind(RenderPhase)
      .each(systemRenderGUI);
  world.system<Window>("Render Window").kind(GuiPhase).each(systemRenderWindow);
}

/// @brief Creates and initialises the GUI system
/// @param world
void systemInitialiseGui(flecs::iter &iter) {
  spdlog::info("Creating GUI");

  auto world = iter.world();

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  // Query default monitor resolution
  float dpi_scaling = 1.0f, ddpi, hdpi, vdpi;
  if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0) {
    spdlog::error("Failed to obtain DPI information for display 0: {}",
                  SDL_GetError());
    // throw std::runtime_error("Failed to obtain DPI information");
  } else {
    dpi_scaling = ddpi / 72.f;
  }

  // Set up Nerd Font
  ImVector<ImWchar> ranges;
  ImFontGlyphRangesBuilder builder;
  ImWchar fa_range[] = {0xed00, 0xf2ff, 0}; // Font Awesome
  ImWchar fae_range[] = {0xe200, 0x2a9, 0}; // Font Awesome Extended
  ImWchar md_range[] = {0xf001, static_cast<ImWchar>(0xf1af0),
                        0};                 // Material Design Icons
  ImWchar wt_range[] = {0xe300, 0xe3e, 0};  // Weather Icons
  ImWchar oc_range[] = {0xf400, 0xf533, 0}; // Octicon Icons
  ImWchar ha_range[] = {0x276c, 0x2771, 0}; // Heavy Angle Bracket Icons
  ImWchar bd_range[] = {0x2500, 0x259f, 0}; // Box Drawing Icons
  ImWchar pg_range[] = {0xee00, 0xee0b, 0}; // Progress Icons

  builder.AddRanges(io.Fonts->GetGlyphRangesDefault()); // Add the default range
  builder.AddRanges(fa_range);
  builder.AddRanges(fae_range);
  builder.AddRanges(md_range);
  builder.AddRanges(wt_range);
  builder.AddRanges(oc_range);
  builder.AddRanges(ha_range);
  builder.AddRanges(bd_range);
  builder.AddRanges(pg_range);
  builder.BuildRanges(&ranges);

  auto default_font = world.lookup("Fonts::Default");
  SC_ASSERT(default_font.is_valid(), "Default font entity not found");
  auto font = default_font.get<Font>();

  ImFontConfig config;
  io.Fonts->AddFontFromFileTTF(
      font.name.c_str(), dpi_scaling * static_cast<float>(font.point_size),
      &config, ranges.Data);
  io.Fonts->Build();

  auto r = world.get_mut<Renderer>();
  ImGui_ImplSDL2_InitForSDLRenderer(r.window, r.renderer);
  ImGui_ImplSDLRenderer2_Init(r.renderer);
  ImGui_ImplSDLRenderer2_CreateFontsTexture();

  world.entity("Windows");
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

flecs::entity showWindow(flecs::world &world, const std::string &name) {
  auto windows = world.lookup("Windows");
  if (!windows.is_valid()) {
    windows = world.entity("Windows");
  }
  auto winE = windows.lookup(name.c_str());
  if (!winE.is_valid()) {
    spdlog::warn("Tried to show window '{}' but it does not exist", name);
    return flecs::entity();
  }
  winE.get_mut<Window>().open = true;
  return winE;
}

flecs::entity hideWindow(flecs::world &world, const std::string &name) {
  auto windows = world.lookup("Windows");
  if (!windows.is_valid()) {
    windows = world.entity("Windows");
  }
  auto winE = windows.lookup(name.c_str());
  if (!winE.is_valid()) {
    spdlog::warn("Tried to hide window '{}' but it does not exist", name);
    return flecs::entity();
  }
  winE.get_mut<Window>().open = false;
  return winE;
}

flecs::entity
registerWindow(std::string name,
               std::function<void(flecs::entity)> content_renderer,
               flecs::world &world) {
  spdlog::info("Registering Window '{}'", name);
  auto windows = world.lookup("Windows");
  if (!windows.is_valid()) {
    windows = world.entity("Windows");
  }
  return world.entity(name.c_str())
      .set<Window>({false, content_renderer})
      .child_of(windows);
}

void systemRenderWindow(flecs::entity winE, Window &win) {
  if (!win.open) {
    return;
  }

  ImGui::Begin(winE.name().c_str(), &win.open);
  if (win.content_renderer) {
    win.content_renderer(winE);
  } else {
    spdlog::warn("Window '{}' has no content renderer set",
                 winE.name().c_str());
  }
  ImGui::End();
}
