#include "input.h"
#include "backends/imgui_impl_sdl2.h"
#include "modules/phase.h"
#include "modules/simulation.h"
#include "modules/site.h"
#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>

void systemEventHandling(flecs::iter &);

InputModule::InputModule(flecs::world &world) {
  world.import <PhaseModule>();

  // Register components

  // Register systems
  world.system("Event Handling").kind(PreFramePhase).run(systemEventHandling);
}

/// @brief SDL-based event handler and dispatcher
void systemEventHandling(flecs::iter &it) {
  // spdlog::info("Handling Events");
  auto world = it.world();
  auto sim = world.get_mut<Simulation>();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
      // User requests quit
    case SDL_QUIT:
      world.quit();
      break;
    case SDL_KEYUP:
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        world.quit();
      }
      if (event.key.keysym.sym == SDLK_SPACE) {
        sim->speed.enabled() ? sim->speed.disable() : sim->speed.enable();
      }
      if (event.key.keysym.sym == SDLK_l) {
        showSiteWindow(world.lookup("cape_canaveral"));
      }
      break;

    default:
      break;
    }
  }
}
