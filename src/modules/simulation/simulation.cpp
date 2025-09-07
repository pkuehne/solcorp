#include "simulation.h"
#include "SDL_keycode.h"
#include "flecs/addons/cpp/c_types.hpp"
#include "modules/engine/engine.h"
#include "modules/engine/input.h"
#include "modules/lua/lua.h"
#include "modules/simulation/developer_window.h"
#include "spdlog/spdlog.h"

void systemUpdateSimDate(Game &game);
void systemQuitOnEscape(flecs::iter &, size_t, const KeyDown);
void systemShowDeveloperWindow(flecs::iter &, size_t, const KeyDown);
void systemModCallbackForUpdate(flecs::entity, Mod &);
void systemModCallbackForFrame(flecs::entity, Mod &);

SimulationModule::SimulationModule(flecs::world &world) {
  world.import <EngineModule>();

  // Register components
  world.component<Simulation>()
      .member("speed", &Simulation::speed)
      .add(flecs::Singleton);
  world.component<Game>().member("day", &Game::day).add(flecs::Singleton);
  world.component<Developer>()
      .member("show_metrics_window", &Developer::show_metrics_window)
      .add(flecs::Singleton);
  world.component<DeveloperWindow>().member("open", &DeveloperWindow::open);

  // Create Singletons
  auto sim = Simulation{world.timer("SimTimer").interval(0.5f).disable()};
  world.set<Simulation>(sim);
  world.set<Game>({});
  world.set<Developer>({});

  register_lua_user_type<Game>(
      world, "Game",
      [](sol::usertype<Game> &userType) { userType["day"] = &Game::day; });

  // Register systems
  world.system<Game>("Update Simulation Date")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemUpdateSimDate);

  world.system<const KeyDown>("Quit on Esc")
      .kind(ValidatePhase)
      .each(systemQuitOnEscape);
  world.system<const KeyDown>("Show Developer Window")
      .kind(ValidatePhase)
      .each(systemShowDeveloperWindow);
  world.system<DeveloperWindow>("Draw Developer Window")
      .kind(GuiPhase)
      .each(systemDrawDeveloperWindow);
  world.system<Mod>("Mod on_update Event")
      .kind(UpdatePhase)
      .tick_source(sim.speed)
      .each(systemModCallbackForUpdate);
  world.system<Mod>("Mod on_frame Event")
      .kind(UpdatePhase)
      .each(systemModCallbackForFrame);
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

void systemShowDeveloperWindow(flecs::iter &it, size_t, const KeyDown event) {
  auto world = it.world();
  if (event.key == SDLK_d) {
    showDeveloperWindow(world);
  }
}

void systemModCallbackForUpdate(flecs::entity e, Mod &mod) {
  auto world = e.world();

  run_mod_handler(mod, world, "on_update");
}

void systemModCallbackForFrame(flecs::entity e, Mod &mod) {
  auto world = e.world();

  run_mod_handler(mod, world, "on_frame");
}
