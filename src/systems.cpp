#include "systems.h"
#include "backends/imgui_impl_sdl2.h"
#include "modules/simulation.h"
#include "modules/site.h"
#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>

void systemEventHandling(flecs::iter &it, size_t, Simulation &sim) {
  // spdlog::info("Handling Events");

  auto world = it.world();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    // if (r->gui->handleEvent(event))
    //   continue; // Gui handled the event

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
        sim.speed.enabled() ? sim.speed.disable() : sim.speed.enable();
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
