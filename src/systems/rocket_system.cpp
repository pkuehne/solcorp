#include "rocket_system.h"
#include "components.h"
#include "spdlog/spdlog.h"
// #include <iostream>
//  #include <sstream>

void rocket_generator_system(flecs::iter &it) {
  spdlog::debug("Creating rockets");
  const flecs::world &world = it.world();

  auto prefabs = world.get<PrefabResource>();
  world.entity().is_a(prefabs->rocket);
}

void rocket_print_system(flecs::iter &, size_t, const Rocket &) {
  // auto entity = it.entity(i);
  // auto cargo_space = entity.get<CargoHold>();
  // std::stringstream output;
  // output << "ID: " << it.entity(i).id();
  // output << " Cargo Space: " << cargo_space->capacity;
  // if (entity.has<LaunchingWith>(flecs::Wildcard)) {
  //   auto launch = entity.target<LaunchingWith>();
  //   output << " Launch: " << launch.get<LaunchPlan>()->remaining_days
  //          << " days";
  // }
  // // std::cout << " Type: [" << entity.type().str() << "]";
  // spdlog::info(output.str());
}

void rocket_launch_scheduler_system(flecs::iter &, size_t, const Rocket &) {
  // if (i > 1) {
  //   return;
  // }
  // auto world = it.world();
  // auto rocket = it.entity(i);
  // auto launch = world.entity().set<PlanetaryLaunch>({10});
  // rocket.add<LaunchingWith>(launch);
  // spdlog::info("Launching rocket {}", rocket.id());
  //
  // auto system = world.lookup("rocket_launch_scheduler_system");
  // // std::cout << system.type().str() << std::endl;
  // system.disable();
}

void launch_decrementer_system(LaunchPlan &) {
  // launch.remaining_days--;
}

void systemLaunchRocket(flecs::entity planE, LaunchPlan &plan) {
  auto world = planE.world();
  u_int today = world.get<GameResource>()->day;

  if (plan.launch_date > today) {
    return;
  }

  auto rocketE = planE.target<LaunchingOn>();
  if (rocketE.is_valid()) {
    spdlog::info("Removing rocket: {}", rocketE.id());
    rocketE.destruct();
  }
  auto payloadE = planE.target<LaunchingWith>();
  if (payloadE.is_valid()) {
    spdlog::info("Removing payload: {}", payloadE.id());

    payloadE.destruct();
  }
  spdlog::info("Removing plan: {}", planE.id());
  planE.destruct();
}
