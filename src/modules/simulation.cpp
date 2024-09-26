#include "simulation.h"
#include "spdlog/spdlog.h"

void systemUpdateSimDate(Game &game);

SimulationModule::SimulationModule(flecs::world &world) {
  // Register components
  world.component<Simulation>().member<flecs::entity>("speed");
  world.component<Game>().member<u_int>("day");

  // Create Singletons
  auto sim = Simulation{world.timer("SimTimer").interval(0.5f).disable()};
  world.set<Simulation>(sim);
  world.set<Game>({});

  // Register systems
  flecs::entity UpdatePhase = world.lookup("Phase.Update");
  world.system<Game>("Update Simulation Date")
      .tick_source(sim.speed)
      .term_at(0)
      .singleton()
      .kind(UpdatePhase)
      .each(systemUpdateSimDate);
}

void systemUpdateSimDate(Game &game) {
  game.day++;
  spdlog::info("It's Day {}", game.day);
}
