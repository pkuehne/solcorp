#include "actions.h"
#include "modules/simulation/simulation.h"
#include "rocket_launch.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

ValidationResult PlannedLaunch::validate(const flecs::world &world) const {
  const u_int launchPrepDays = 5;
  u_int today = world.get<Game>().day;

  if (world.lookup(name.c_str()) != current) {
    return ValidationResult::Fail(
        fmt::format("A Launch Plan named '{}' already exists", name));
  }
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("No rocket selected");
  } else if (rocket.has<LaunchingOn>(flecs::Wildcard)) {
    // BUG: This doesn't account for the rocket being planned for *this*
    // existing launch
    return ValidationResult::Fail("Rocket is already planned for a launch");
  }
  if (!launchpad.is_valid()) {
    return ValidationResult::Fail("No launchpad selected");
  } else {
    bool clash = false;
    launchpad.each<LaunchingFrom>([&](flecs::entity p) {
      auto launch = p.get<LaunchPlan>();
      if (launch.launch_date < static_cast<u_int>(launchDay) &&
          launch.launch_date >= (launchDay - launchPrepDays)) {
        clash = true;
      }
    });
    if (clash) {
      return ValidationResult::Fail(
          "Another launch is already scheduled at that time");
    }
  }
  if (static_cast<u_int>(launchDay) < today + launchPrepDays) {
    return ValidationResult::Fail(
        fmt::format("Launch needs to be planned at least {} days in advance",
                    launchPrepDays));
  }

  return ValidationResult::Pass();
}

void PlannedLaunch::execute(flecs::world &world) {
  auto plan = LaunchPlan();
  auto planE = world.entity().set<LaunchPlan>(plan);
  plan.launch_date = launchDay;
  planE.set_name(name.c_str());
  planE.add<LaunchingOn>(rocket);
  planE.add<LaunchingFrom>(launchpad);
  result = planE;
}
