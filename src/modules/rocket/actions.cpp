#include "actions.h"
#include "modules/base/action.h"
#include "modules/base/base.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "rocket_launch.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

ValidationResult
ScheduleLaunchAction::validate(const flecs::world &world) const {
  uint32_t today = world.get<Game>().day;

  flecs::entity existing = world.lookup(name.c_str());
  if (existing.is_valid()) {
    return ValidationResult::Fail(
        fmt::format("A Launch Plan named '{}' already exists", name));
  }
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("No rocket selected");
  }
  if (rocket.get<RocketState>().current != RocketStateId::Stored) {
    return ValidationResult::Fail("Selected rocket is not unassigned");
  }
  if (rocket.has<LaunchingOn>(flecs::Wildcard)) {
    return ValidationResult::Fail("Rocket is already planned for a launch");
  }
  if (!launchpad.is_valid()) {
    return ValidationResult::Fail("No launchpad selected");
  }
  auto launchPrepDays =
      static_cast<uint32_t>(launchpad.get<Launchpad>().prep_days.value());
  bool clash = false;
  launchpad.each<LaunchingFrom>([&](flecs::entity p) {
    auto launch = p.get<LaunchPlan>();
    if (std::cmp_less(launch.launch_date, launchDay) &&
        std::cmp_greater_equal(launch.launch_date,
                               static_cast<uint32_t>(launchDay) -
                                   launchPrepDays)) {
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
  auto planData = LaunchPlan{.launch_date = static_cast<uint32_t>(launchDay),
                             .target_orbit = targetOrbit};
  auto planE = world.entity().set<LaunchPlan>(planData);
  planE.set_name(name.c_str());
  planE.add<LaunchingOn>(rocket);
  planE.add<LaunchingFrom>(launchpad);
  rocket.get_mut<RocketState>().current = RocketStateId::Assigned;
  for (const auto &payload : payloads) {
    if (payload.is_valid()) {
      planE.add<LaunchingWith>(payload);
    }
  }
  result = planE;
}

ValidationResult EditLaunchAction::validate(const flecs::world &world) const {
  uint32_t today = world.get<Game>().day;

  if (!plan.is_valid() || !plan.is_alive()) {
    return ValidationResult::Fail("No plan to edit");
  }
  flecs::entity existing = world.lookup(name.c_str());
  if (existing.is_valid() && existing != plan) {
    return ValidationResult::Fail(
        fmt::format("A Launch Plan named '{}' already exists", name));
  }
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("No rocket selected");
  }
  bool same_rocket = (rocket == plan.target<LaunchingOn>());
  if (!same_rocket &&
      rocket.get<RocketState>().current != RocketStateId::Stored) {
    return ValidationResult::Fail("Selected rocket is not unassigned");
  }
  if (!same_rocket && rocket.has<LaunchingOn>(flecs::Wildcard)) {
    return ValidationResult::Fail("Rocket is already planned for a launch");
  }
  if (!launchpad.is_valid()) {
    return ValidationResult::Fail("No launchpad selected");
  }
  auto launchPrepDays =
      static_cast<uint32_t>(launchpad.get<Launchpad>().prep_days.value());
  bool clash = false;
  launchpad.each<LaunchingFrom>([&](flecs::entity p) {
    if (p == plan) {
      return;
    }
    auto launch = p.get<LaunchPlan>();
    if (std::cmp_less(launch.launch_date, launchDay) &&
        std::cmp_greater_equal(launch.launch_date,
                               static_cast<uint32_t>(launchDay) -
                                   launchPrepDays)) {
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

void EditLaunchAction::execute(flecs::world &world) {
  CancelLaunchAction{plan}.execute(world);
  auto planData = LaunchPlan{.launch_date = static_cast<uint32_t>(launchDay),
                             .target_orbit = targetOrbit};
  auto planE = world.entity().set<LaunchPlan>(planData);
  planE.set_name(name.c_str());
  planE.add<LaunchingOn>(rocket);
  planE.add<LaunchingFrom>(launchpad);
  rocket.get_mut<RocketState>().current = RocketStateId::Assigned;
  for (const auto &payload : payloads) {
    if (payload.is_valid()) {
      planE.add<LaunchingWith>(payload);
    }
  }
  result = planE;
}

ValidationResult CancelLaunchAction::validate(const flecs::world &) const {
  if (!plan.is_valid() || !plan.is_alive()) {
    return ValidationResult::Fail("Launch plan is not valid");
  }
  return ValidationResult::Pass();
}

void CancelLaunchAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }
  auto rocketE = plan.target<LaunchingOn>();
  if (rocketE.is_valid()) {
    rocketE.get_mut<RocketState>().current = RocketStateId::Stored;
  }
  spdlog::debug("Cancelling launch plan: {}", plan.id());
  plan.destruct();
}

ValidationResult BuildRocketAction::validate(const flecs::world &world) const {
  if (!prefab.is_valid()) {
    return ValidationResult::Fail("Rocket prefab is not valid");
  }
  if (!line.is_valid()) {
    return ValidationResult::Fail("Manufacturing line is not valid");
  }

  auto &company = world.get_mut<Company>();
  if (company.balance < this->cost) {
    return ValidationResult::Fail("Not enough funds to build this rocket");
  }

  return ValidationResult::Pass();
}

void BuildRocketAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  auto rocket =
      world.entity()
          .is_a(this->prefab)
          .set<Construction>({.effort_remaining = 300, .effort_total = 300})
          .child_of(this->line);
  rocket.set_name(fmt::format("Rocket {}", Rocket::max_id++).c_str());

  // deduct cost
  auto &company = world.get_mut<Company>();
  company.balance -= this->cost;
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
