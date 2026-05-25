#include "launch_actions.h"
#include "modules/base/action.h"
#include "modules/base/base.h"
#include "modules/site/site.h"
#include "rocket_actions.h"
#include "rocket_module.h"
#include <flecs.h>
#include <spdlog/spdlog.h>

ValidationResult
LaunchScheduleAction::validate(const flecs::world &world) const {
  uint32_t today = world.get<Game>().day;

  flecs::entity existing = world.lookup(name.c_str());
  if (existing.is_valid()) {
    return ValidationResult::Fail(
        fmt::format("A Launch Plan named '{}' already exists", name));
  }
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("No rocket selected");
  }
  if (!rocket.has<RocketCurrentState>(world.lookup("States::Rocket::Stored"))) {
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
  auto rolloutDays =
      static_cast<uint32_t>(rocket.get<Rocket>().rollout_days.value());
  auto newPrepDate = static_cast<uint32_t>(launchDay) - launchPrepDays;
  // TODO(#95): Remove this when refactoring, rockets shouldn't know about
  // launchpads
  bool clash = false;
  launchpad.each<LaunchingFrom>([&](flecs::entity p) {
    auto &plan = p.get<LaunchPlan>();
    auto existingPrepDate = plan.launch_date >= launchPrepDays
                                ? plan.launch_date - launchPrepDays
                                : 0;
    if (newPrepDate <= plan.launch_date &&
        existingPrepDate <= static_cast<uint32_t>(launchDay)) {
      clash = true;
    }
  });
  if (clash) {
    return ValidationResult::Fail(
        "Another launch is already scheduled at that time");
  }
  if (static_cast<uint32_t>(launchDay) < today + launchPrepDays + rolloutDays) {
    return ValidationResult::Fail(
        fmt::format("Launch needs to be planned at least {} days in advance",
                    launchPrepDays + rolloutDays));
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

void LaunchScheduleAction::execute(flecs::world &world) {
  auto prepDays =
      static_cast<uint32_t>(launchpad.get<Launchpad>().prep_days.value());
  auto rollDays =
      static_cast<uint32_t>(rocket.get<Rocket>().rollout_days.value());
  auto prepDate = static_cast<uint32_t>(launchDay) - prepDays;
  auto planData = LaunchPlan{.rollout_date = prepDate - rollDays,
                             .prep_date = prepDate,
                             .launch_date = static_cast<uint32_t>(launchDay),
                             .target_orbit = targetOrbit};
  auto planE = world.entity().set<LaunchPlan>(planData);
  planE.set_name(name.c_str());
  planE.add<LaunchingOn>(rocket);
  planE.add<LaunchingFrom>(launchpad);
  planE.add<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::Scheduled"));
  rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
  for (const auto &payload : payloads) {
    if (payload.is_valid()) {
      planE.add<LaunchingWith>(payload);
    }
  }
  result = planE;
}

ValidationResult LaunchEditAction::validate(const flecs::world &world) const {
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
      !rocket.has<RocketCurrentState>(world.lookup("States::Rocket::Stored"))) {
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
  auto rolloutDays =
      static_cast<uint32_t>(rocket.get<Rocket>().rollout_days.value());
  auto newPrepDate = static_cast<uint32_t>(launchDay) - launchPrepDays;
  bool clash = false;
  launchpad.each<LaunchingFrom>([&](flecs::entity p) {
    if (p == plan) {
      return;
    }
    auto &existingPlan = p.get<LaunchPlan>();
    auto existingPrepDate = existingPlan.launch_date >= launchPrepDays
                                ? existingPlan.launch_date - launchPrepDays
                                : 0;
    if (newPrepDate <= existingPlan.launch_date &&
        existingPrepDate <= static_cast<uint32_t>(launchDay)) {
      clash = true;
    }
  });
  if (clash) {
    return ValidationResult::Fail(
        "Another launch is already scheduled at that time");
  }
  if (static_cast<uint32_t>(launchDay) < today + launchPrepDays + rolloutDays) {
    return ValidationResult::Fail(
        fmt::format("Launch needs to be planned at least {} days in advance",
                    launchPrepDays + rolloutDays));
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

void LaunchEditAction::execute(flecs::world &world) {
  LaunchCancelAction{plan}.execute(world);
  auto prepDays =
      static_cast<uint32_t>(launchpad.get<Launchpad>().prep_days.value());
  auto rollDays =
      static_cast<uint32_t>(rocket.get<Rocket>().rollout_days.value());
  auto prepDate = static_cast<uint32_t>(launchDay) - prepDays;
  auto planData = LaunchPlan{.rollout_date = prepDate - rollDays,
                             .prep_date = prepDate,
                             .launch_date = static_cast<uint32_t>(launchDay),
                             .target_orbit = targetOrbit};
  auto planE = world.entity().set<LaunchPlan>(planData);
  planE.set_name(name.c_str());
  planE.add<LaunchingOn>(rocket);
  planE.add<LaunchingFrom>(launchpad);
  rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Assigned"));
  for (const auto &payload : payloads) {
    if (payload.is_valid()) {
      planE.add<LaunchingWith>(payload);
    }
  }
  result = planE;
}

ValidationResult LaunchCancelAction::validate(const flecs::world &) const {
  if (!plan.is_valid() || !plan.is_alive()) {
    return ValidationResult::Fail("Launch plan is not valid");
  }
  return ValidationResult::Pass();
}

void LaunchCancelAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }
  auto rocketE = plan.target<LaunchingOn>();
  if (rocketE.is_valid()) {
    rocketE.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
  }
  spdlog::debug("Cancelling launch plan: {}", plan.id());
  plan.destruct();
}

// ----- LaunchInitiateRolloutAction -----

ValidationResult
LaunchInitiateRolloutAction::validate(const flecs::world &world) const {
  if (!plan.is_valid() || !plan.is_alive()) {
    return ValidationResult::Fail("Launch plan is not valid");
  }
  if (!plan.has<LaunchPlanCurrentState>(
          world.lookup("States::LaunchPlan::Scheduled"))) {
    return ValidationResult::Fail("Launch plan is not in Scheduled state");
  }
  auto rocketE = plan.target<LaunchingOn>();
  if (!rocketE.is_valid()) {
    return ValidationResult::Fail("Launch plan has no rocket assigned");
  }
  if (!rocketE.has<RocketCurrentState>(
          world.lookup("States::Rocket::Assigned"))) {
    return ValidationResult::Fail("Rocket is not in Assigned state");
  }
  if (!plan.target<LaunchingFrom>().is_valid()) {
    return ValidationResult::Fail("Launch plan has no launchpad assigned");
  }
  return ValidationResult::Pass();
}

void LaunchInitiateRolloutAction::execute(flecs::world &world) {
  auto rocketE = plan.target<LaunchingOn>();
  auto launchpadE = plan.target<LaunchingFrom>();
  auto rolloutDays =
      static_cast<uint8_t>(rocketE.get<Rocket>().rollout_days.value());
  RocketMoveAction{RocketEntity{rocketE}, DestinationEntity{launchpadE},
                   rolloutDays}
      .execute(world);
  rocketE.add<RocketTargetState>(world.lookup("States::Rocket::Assigned"));
  plan.add<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::RollingOut"));
}

// ----- LaunchCompleteRolloutAction -----

ValidationResult
LaunchCompleteRolloutAction::validate(const flecs::world &world) const {
  if (!plan.is_valid() || !plan.is_alive()) {
    return ValidationResult::Fail("Launch plan is not valid");
  }
  if (!plan.has<LaunchPlanCurrentState>(
          world.lookup("States::LaunchPlan::RollingOut"))) {
    return ValidationResult::Fail("Launch plan is not rolling out");
  }
  auto rocketE = plan.target<LaunchingOn>();
  auto launchpadE = plan.target<LaunchingFrom>();
  if (!rocketE.is_valid()) {
    return ValidationResult::Fail("Launch plan has no rocket assigned");
  }
  if (rocketE.parent() != launchpadE) {
    return ValidationResult::Fail(
        "Rocket has not yet arrived at the launchpad");
  }
  return ValidationResult::Pass();
}

void LaunchCompleteRolloutAction::execute(flecs::world &world) {
  auto launchpadE = plan.target<LaunchingFrom>();
  auto prepDays =
      static_cast<uint32_t>(launchpadE.get<Launchpad>().prep_days.value());
  plan.set<DurationRequired>({.remaining = prepDays, .total = prepDays});
  plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
}

// ----- LaunchAction -----

ValidationResult LaunchGoAction::validate(const flecs::world &world) const {
  if (!plan.is_valid() || !plan.is_alive()) {
    return ValidationResult::Fail("Launch plan is not valid");
  }
  if (!plan.has<LaunchPlanCurrentState>(
          world.lookup("States::LaunchPlan::OnPad"))) {
    return ValidationResult::Fail("Launch plan is not in preparation");
  }
  if (plan.has<DurationRequired>()) {
    return ValidationResult::Fail("Launch preparation is not yet complete");
  }
  return ValidationResult::Pass();
}

void LaunchGoAction::execute(flecs::world &world) {
  plan.add<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::Launched"));
}
