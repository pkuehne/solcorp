#include "rocket_system.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>

void rocket_generator_system(flecs::iter &it) {
  spdlog::debug("Creating rockets");
  const flecs::world &world = it.world();

  auto prefabs = world.get<PrefabResource>();
  world.entity().is_a(prefabs->rocket);
}

void rocket_print_system(flecs::iter &it, size_t i, const Rocket &) {
  auto entity = it.entity(i);
  auto cargo_space = entity.get<CargoHold>();
  std::stringstream output;
  output << "ID: " << it.entity(i).id();
  output << " Cargo Space: " << cargo_space->capacity;
  if (entity.has<LaunchingWith>(flecs::Wildcard)) {
    auto launch = entity.target<LaunchingWith>();
    output << " Launch: " << launch.get<PlanetaryLaunch>()->remaining_days
           << " days";
  }
  // std::cout << " Type: [" << entity.type().str() << "]";
  spdlog::info(output.str());
}

void rocket_launch_scheduler_system(flecs::iter &it, size_t i, const Rocket &) {
  if (i > 1) {
    return;
  }
  auto world = it.world();
  auto rocket = it.entity(i);
  auto launch = world.entity().set<PlanetaryLaunch>({10});
  rocket.add<LaunchingWith>(launch);
  spdlog::info("Launching rocket {}", rocket.id());

  auto system = world.lookup("rocket_launch_scheduler_system");
  // std::cout << system.type().str() << std::endl;
  system.disable();
}

void launch_decrementer_system(PlanetaryLaunch &launch) {
  launch.remaining_days--;
}

void launch_end_system(flecs::entity &entity, PlanetaryLaunch &launch) {
  if (launch.remaining_days > 0 || launch.completed) {
    return;
  }
  launch.completed = true;

  auto rocket = entity.target<LaunchingWith>();
  entity.remove<LaunchingWith>(rocket);
  spdlog::info("{} has launched", rocket.id());
  rocket.destruct();
}
