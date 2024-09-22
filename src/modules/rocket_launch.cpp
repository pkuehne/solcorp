
#include "rocket_launch.h"
#include "components.h"
#include "spdlog/spdlog.h"
#include <flecs.h>

RocketLaunchModule::RocketLaunchModule(flecs::world &world) {
  auto game = world.get<GameResource>();
  flecs::entity UpdatePhase = world.lookup("Phases.Update");

  // Register components
  world.component<LaunchingWith>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingFrom>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingOn>().add(flecs::Exclusive).add(flecs::Symmetric);

  // Register systems
  world.system<LaunchPlan>("Launch Rocket")
      .tick_source(game->sim_speed)
      .kind(UpdatePhase)
      .each(systemLaunchRocket);
}

/// @brief Process LaunchPlans that are due
/// Ensures that the rocket is destroyed after being launched
/// Also removes the launchplan and clears all relationships
/// @param planE The plan's entity
/// @param plan The plan's component
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
