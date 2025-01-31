#include "simulation.h"
#include "SDL_keycode.h"
#include "modules/engine/engine.h"
#include "modules/engine/input.h"
#include "spdlog/spdlog.h"

void systemUpdateSimDate(Game &game);
void systemQuitOnEscape(flecs::iter &, size_t, const KeyDown);

SimulationModule::SimulationModule(flecs::world &world) {
  world.import <EngineModule>();

  // Register components
  world.component<Simulation>().member<flecs::entity>("speed");
  world.component<Game>().member<u_int>("day");

  // Create Singletons
  auto sim = Simulation{world.timer("SimTimer").interval(0.5f).disable()};
  world.set<Simulation>(sim);
  world.set<Game>({});

  // Register systems
  world.system<Game>("Update Simulation Date")
      .tick_source(sim.speed)
      .term_at(0)
      .singleton()
      .kind(UpdatePhase)
      .each(systemUpdateSimDate);

  world.system<const KeyDown>("Quit on Esc")
      .term_at(0)
      .singleton()
      .kind(ValidatePhase)
      .each(systemQuitOnEscape);
}

void systemUpdateSimDate(Game &game) {
  game.day++;
  spdlog::info("It's Day {}", game.day);
}

void systemQuitOnEscape(flecs::iter &it, size_t, const KeyDown event) {
  if (event.key == SDLK_ESCAPE) {
    it.world().quit();
  }
}
