#include "actions.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "rocket_launch.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

ValidationResult
ScheduleLaunchAction::validate(const flecs::world &world) const {
  uint32_t today = world.get<Game>().day;

  flecs::entity existing = world.lookup(name.c_str());
  if (existing.is_valid() && existing != current) {
    return ValidationResult::Fail(
        fmt::format("A Launch Plan named '{}' already exists", name));
  }
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("No rocket selected");
  }
  if (current.is_valid() && rocket.has<LaunchingOn>(current)) {
    // Rocket is already planned for this launch
  } else if (rocket.has<LaunchingOn>(flecs::Wildcard)) {
    // Rocket is planned for another launch
    return ValidationResult::Fail("Rocket is already planned for a launch");
  }
  if (!launchpad.is_valid()) {
    return ValidationResult::Fail("No launchpad selected");
  }
  unsigned int launchPrepDays = launchpad.get<Launchpad>().prep_days.value();
  bool clash = false;
  launchpad.each<LaunchingFrom>([&](flecs::entity p) {
    auto launch = p.get<LaunchPlan>();
    if (launch.launch_date < static_cast<uint32_t>(launchDay) &&
        launch.launch_date >= (launchDay - launchPrepDays)) {
      clash = true;
    }
  });
  if (clash) {
    return ValidationResult::Fail(
        "Another launch is already scheduled at that time");
  }

  if (static_cast<uint32_t>(launchDay) < today + launchPrepDays) {
    return ValidationResult::Fail(
        fmt::format("Launch needs to be planned at least {} days in advance",
                    launchPrepDays));
  }

  if (!targetOrbit.is_valid()) {
    return ValidationResult::Fail("No target orbit selected");
  }
  if (!rocket.has<CanLiftTo>(targetOrbit)) {
    return ValidationResult::Fail(
        "Rocket cannot lift payload to selected orbit");
  }

  // Check total payload mass
  uint32_t totalMass = 0;
  for (const auto &payload : payloads) {
    if (payload.is_valid() && payload.has<Payload>()) {
      totalMass += payload.get<Payload>().mass;
    }
  }
  auto maxMass = rocket.get<CanLiftTo>(targetOrbit).max_mass;
  if (totalMass > maxMass) {
    return ValidationResult::Fail(fmt::format(
        "Total payload mass {} kg exceeds maximum {} kg", totalMass, maxMass));
  }

  return ValidationResult::Pass();
}

void ScheduleLaunchAction::execute(flecs::world &world) {
  if (current.is_valid()) {
    spdlog::debug("Removing existing launch plan: {}", current.id());
    current.destruct();
  }
  auto plan = LaunchPlan{.launch_date = static_cast<uint32_t>(launchDay),
                         .target_orbit = targetOrbit};

  auto planE = world.entity().set<LaunchPlan>(plan);
  planE.set_name(name.c_str());
  planE.add<LaunchingOn>(rocket);
  planE.add<LaunchingFrom>(launchpad);

  // Add payloads to the plan
  for (const auto &payload : payloads) {
    if (payload.is_valid()) {
      planE.add<LaunchingWith>(payload);
    }
  }

  result = planE;
}

ValidationResult MoveRocketAction::validate(const flecs::world &) const {
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("Rocket is not valid");
  }
  if (rocket.has<Construction>()) {
    return ValidationResult::Fail("Rocket is under construction");
  }
  if (!destination.is_valid()) {
    return ValidationResult::Fail("Destination is not valid");
  }
  if (rocket.parent() == destination) {
    return ValidationResult::Fail(
        "Rocket is already stored in the selected destination");
  }
  return ValidationResult::Pass();
}

void MoveRocketAction::execute(flecs::world &world) {
  if (!!validate(world)) {
    rocket.child_of(destination);
  }
}
