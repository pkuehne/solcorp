#include "systems.h"
#include "backends/imgui_impl_sdl2.h"
#include "components.h"
#include "graphics.h"
#include "spdlog/spdlog.h"
#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>

void systemEventHandling(flecs::iter &it, GameResource *game, Renderer *) {
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
        game->sim_speed.enabled() ? game->sim_speed.disable()
                                  : game->sim_speed.enable();
      }
      if (event.key.keysym.sym == SDLK_l) {
        auto gui = world.get_mut<GuiResource>();
        gui->site_window.visible = !gui->site_window.visible;
      }
      break;

    default:
      break;
    }
  }
}

void systemUpdateSimDate(flecs::iter &, GameResource *game) {
  game->day++;
  spdlog::info("It's Day {}", game->day);
}

void company_generator_system(flecs::iter &it) {
  spdlog::debug("Creating teams and employees");
  const flecs::world &world = it.world();

  auto finance = world.entity("Finance").set<Team>({"Finance"});
  auto purchasing = world.entity("Purchasing")
                        .set<Team>({"Purchasing"})
                        .add<TeamMember>(finance);
  auto payable = world.entity("Payable")
                     .set<Team>({"Accounts Payable"})
                     .add<TeamMember>(finance);

  world.entity("Anna")
      .set<Person>({"Anna", "Delaney"})
      .add<Employee>()
      .add<TeamMember>(purchasing);
  world.entity("Bernardo")
      .set<Person>({"Bernardo", "Jimenez"})
      .add<Employee>()
      .add<TeamMember>(purchasing);
  world.entity("Kathy")
      .set<Person>({"Kathy", "Lau"})
      .add<Employee>()
      .add<TeamMember>(purchasing)
      .add<Manager>(purchasing);
  world.entity("Dave")
      .set<Person>({"Dave", "Jones"})
      .add<Employee>()
      .add<TeamMember>(finance)
      .add<Manager>(finance);
  world.entity("Elena")
      .set<Person>({"Elena", "Rodriguez"})
      .add<Employee>()
      .add<TeamMember>(payable)
      .add<Manager>(payable);
  world.entity("Freddie")
      .set<Person>({"Fred", "Dorn"})
      .add<Employee>()
      .add<TeamMember>(payable);
}
