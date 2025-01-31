#include "input.h"
#include "backends/imgui_impl_sdl2.h"
#include "modules/engine/engine.h"
#include <SDL2/SDL.h>
#include <SDL_events.h>

void systemEventHandling(flecs::iter &);
void systemRemoveEvents(flecs::iter &);

InputModule::InputModule(flecs::world &world) {
  world.import <EngineModule>();

  // Register components
  world.component<KeyDown>().member<int>("key");
  world.component<KeyUp>().member<int>("key");
  world.component<KeyPressed>(); //.member<std::map<int, bool>>("keys");

  // Register systems
  world.system("Event Handling").kind(PreFramePhase).run(systemEventHandling);
}

/// @brief SDL-based event handler and dispatcher
void systemEventHandling(flecs::iter &it) {
  // spdlog::info("Handling Events");
  auto world = it.world();
  world.remove<KeyDown>();
  world.remove<KeyUp>();
  world.remove<MouseDown>();
  world.remove<MouseUp>();

  SDL_Event event;
  auto io = ImGui::GetIO();

  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
      // User requests quit
    case SDL_QUIT:
      world.quit();
      break;
    case SDL_KEYDOWN:
      if (io.WantCaptureKeyboard) {
        break;
      }
      world.set<KeyDown>({event.key.keysym.sym});
      world.ensure<KeyPressed>().keys[event.key.keysym.sym] = true;
      break;
    case SDL_KEYUP:
      if (io.WantCaptureKeyboard) {
        break;
      }
      world.set<KeyUp>({event.key.keysym.sym});
      world.ensure<KeyPressed>().keys[event.key.keysym.sym] = false;
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (io.WantCaptureMouse) {
        break;
      }
      world.set<MouseDown>(
          {event.button.x, event.button.y, event.button.button});
      break;
    case SDL_MOUSEBUTTONUP:
      if (io.WantCaptureMouse) {
        break;
      }
      world.set<MouseUp>({event.button.x, event.button.y, event.button.button});
      break;
    default:
      break;
    }
  }
}
