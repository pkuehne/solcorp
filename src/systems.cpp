#include "systems.h"
#include "backends/imgui_impl_sdl2.h"
#include "components.h"
#include "modules/site.h"
#include "spdlog/spdlog.h"
#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>

void systemEventHandling(flecs::iter &it, size_t, GameResource &game) {
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
        game.sim_speed.enabled() ? game.sim_speed.disable()
                                 : game.sim_speed.enable();
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

void systemUpdateSimDate(GameResource &game) {
  game.day++;
  spdlog::info("It's Day {}", game.day);
}
