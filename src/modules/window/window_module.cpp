#include "window_module.h"
#include "SDL_keycode.h"
#include "main_menu.h"
#include "modules/base/base.h"
#include "modules/engine/input.h"
#include <flecs.h>
#include <modules/simulation/simulation.h>

WindowModule::WindowModule(flecs::world &world) {

  // Register components
  world.component<MainMenuBar>();

  // Register window
  world.entity("MainMenuBar").add<MainMenuBar>();

  // Register Systems
  world.system<const Simulation, const Game, MainMenuBar>("Draw MainMenu")
      .kind(GuiPhase)
      .each(systemDrawMainMenu);
  world.system<Simulation, const KeyDown>("Toggle Play/Pause")
      .kind(ValidatePhase)
      .each(systemToggle);
  auto sim = world.get<Simulation>();
  world.system<DurationRequired>("Tick DurationRequired")
      .kind(UpdatePhase)
      .tick_source(sim.speed)
      .each(systemTickDurationRequired);
}

void systemToggle(flecs::iter &it, size_t, Simulation &sim,
                  const KeyDown event) {
  if (event.key == SDLK_SPACE) {
    auto simTimer = it.world().timer(sim.speed.id());
    simTimer.get<flecs::Timer>().active ? simTimer.stop() : simTimer.start();
  }
}

void systemTickDurationRequired(flecs::entity, DurationRequired &duration) {
  if (duration.remaining > 0) {
    duration.remaining--;
  }
}
