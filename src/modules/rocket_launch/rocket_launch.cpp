#include "rocket_launch.h"
#include "actions.h"
#include "launch_window.h"
#include "modules/base/base.h"
#include "modules/simulation/simulation.h"
#include "modules/site/helpers.h"
#include "spdlog/spdlog.h"
#include <flecs.h>

u_int LaunchPlan::max_id = 1;
u_int Rocket::max_id = 1;

/// @brief Module Constructor
/// Sets up all necessary components, GUIs and Systems
RocketLaunchModule::RocketLaunchModule(flecs::world &world) {
  spdlog::debug("Loading RocketLaunchModule");

  world.import <BaseModule>();
  world.import <SimulationModule>();

  // Register components
  world.component<ScheduleLaunchAction>("PlannedLaunch")
      .member("name", &ScheduleLaunchAction::name)
      .member("launchDay", &ScheduleLaunchAction::launchDay)
      .member("rocket", &ScheduleLaunchAction::rocket)
      .member("launchpad", &ScheduleLaunchAction::launchpad);
  world.component<Rocket>();
  world.component<CargoHold>().member("capacity", &CargoHold::capacity);
  world.component<LaunchPlan>();
  world.component<LaunchWindow>()
      .member("planE", &LaunchWindow::planE)
      .member("draftPlan", &LaunchWindow::draftPlan);

  // Register relationships
  world.component<LaunchingWith>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingOn>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingFrom>().add(
      flecs::Symmetric); // Not Exclusive because each Launchpad can have
                         // multiple Plans assigned

  // Register prefabs
  world.prefab<Rocket>().set<CargoHold>({1000});

  // Register systems
  auto sim = world.get<Simulation>();
  world.system<LaunchPlan>("Launch Rocket")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemLaunchRocket);

  world.system<LaunchWindow>("Draw LaunchWindow")
      .kind(GuiPhase)
      .each(systemDrawLaunchWindow);
}

/// @brief Process LaunchPlans that are due
/// Ensures that the rocket is destroyed after being launched
/// Also removes the launchplan and clears all relationships
/// @param planE The plan's entity
/// @param plan The plan's component
void systemLaunchRocket(flecs::entity planE, LaunchPlan &plan) {
  auto world = planE.world();
  u_int today = world.get<Game>().day;

  if (plan.launch_date > today) {
    return;
  }

  auto rocketE = planE.target<LaunchingOn>();
  if (rocketE.is_valid()) {
    spdlog::debug("Removing rocket: {}", rocketE.id());
    rocketE.destruct();
  }
  auto payloadE = planE.target<LaunchingWith>();
  if (payloadE.is_valid()) {
    spdlog::debug("Removing payload: {}", payloadE.id());

    payloadE.destruct();
  }
  auto launchpadE = planE.target<LaunchingFrom>();
  spdlog::debug("Removing plan: {} launch_date: {} today: {}", planE.id(),
                plan.launch_date, today);
  instantiateBuildingNotification(
      world, launchpadE, fmt::format("{} launched", planE.name().c_str()));
  planE.destruct();
}
