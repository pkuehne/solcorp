#include "actions.h"
#include "modules/base/action.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "rocket_module.h"
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
  if (rocket.get<Rocket>().state != RocketStateId::Stored) {
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
  // TODO(#95): Remove this when refactoring, rockets shouldn't know about
  // launchpads
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
  rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
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
  if (!same_rocket && rocket.get<Rocket>().state != RocketStateId::Stored) {
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
  rocket.get_mut<Rocket>().state = RocketStateId::Assigned;
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
    rocketE.get_mut<Rocket>().state = RocketStateId::Stored;
  }
  spdlog::debug("Cancelling launch plan: {}", plan.id());
  plan.destruct();
}

ValidationResult RocketBuildAction::validate(const flecs::world &world) const {
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

void RocketBuildAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  auto rocket = world.entity()
                    .is_a(this->prefab)
                    .set<RocketTargetState>({.target = RocketStateId::Stored})
                    .set<EffortRequired>({.remaining = 300, .total = 300})
                    .child_of(this->line);
  rocket.ensure<Rocket>().state = RocketStateId::UnderConstruction;
  rocket.set_name(fmt::format("Rocket {}", Rocket::max_id++).c_str());

  // deduct cost
  auto &company = world.get_mut<Company>();
  company.balance -= this->cost;
}

// ----- RocketCompleteBuildAction-----

ValidationResult
RocketCompleteBuildAction::validate(const flecs::world &) const {
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("Rocket is not valid");
  }
  if (rocket.get<Rocket>().state != RocketStateId::UnderConstruction) {
    return ValidationResult::Fail("Rocket is not under construction");
  }
  if (rocket.has<EffortRequired>()) {
    return ValidationResult::Fail("Rocket construction is not yet complete");
  }
  // TODO: Check there's enough storage space for the new rocket
  return ValidationResult::Pass();
}

void RocketCompleteBuildAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  rocket.get_mut<Rocket>().state = RocketStateId::Stored;
  rocket.remove<RocketTargetState>();
  rocket.remove<RocketStateTransitionBlocked>();

  instantiateNotification(world, "New rocket built",
                          std::format("{} has been built and is now in storage",
                                      rocket.name().c_str()),
                          world.lookup("NotificationCategories::Rocket Build"),
                          NotificationSeverity::Low);
}

void RocketCompleteBuildAction::block(flecs::world &world) {
  auto result = validate(world);
  if (result.ok) {
    return;
  }

  if (!rocket.has<RocketStateTransitionBlocked>()) {
    instantiateNotification(
        world, "Rocket cannot be completed", result.message,
        world.lookup("NotificationCategories::Rocket Build"),
        NotificationSeverity::Important);
  }
  this->rocket.set<RocketStateTransitionBlocked>({.reason = result.message});
}

// ----- RocketMoveAction-----

ValidationResult RocketMoveAction::validate(const flecs::world &) const {
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("Rocket is not valid");
  }
  if (!rocket.has<Rocket>()) {
    return ValidationResult::Fail("This is not a rocket");
  }
  if (rocket.get<Rocket>().state != RocketStateId::Stored) {
    return ValidationResult::Fail(
        "Rocket must be currently stored to be moved");
  }
  if (rocket.has<EffortRequired>()) {
    return ValidationResult::Fail("Rocket has unfinished effort");
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

void RocketMoveAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }
  rocket.add<RocketTargetParent>(destination);
  rocket.get_mut<Rocket>().state = RocketStateId::Moving;
  rocket.set<RocketTargetState>({.target = RocketStateId::Stored});
  rocket.set<DurationRequired>({.remaining = days, .total = days});
}

// ----- RocketMoveCompleteAction-----

ValidationResult
RocketCompleteMoveAction::validate(const flecs::world &) const {
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("Rocket is not valid");
  }
  if (rocket.get<Rocket>().state != RocketStateId::Moving) {
    return ValidationResult::Fail("Rocket is not currently moving");
  }
  if (!rocket.target<RocketTargetParent>().is_valid()) {
    return ValidationResult::Fail("Rocket does not have a valid target");
  }
  if (rocket.has<DurationRequired>()) {
    return ValidationResult::Fail(
        "Rocket has not yet moved to the new location");
  }
  return ValidationResult::Pass();
}

void RocketCompleteMoveAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }
  auto targetParent = rocket.target<RocketTargetParent>();
  instantiateNotification(world, "Rocket move completed",
                          std::format("{} has been moved to {}",
                                      rocket.name().c_str(),
                                      targetParent.name().c_str()),
                          world.lookup("NotificationCategories::Rocket Move"),
                          NotificationSeverity::Low);

  rocket.child_of(targetParent);
  rocket.get_mut<Rocket>().state = rocket.get<RocketTargetState>().target;
  rocket.remove<RocketTargetState>();
  rocket.remove<RocketTargetParent>();
  rocket.remove<RocketStateTransitionBlocked>();
}

void RocketCompleteMoveAction::block(flecs::world &world) {
  auto result = validate(world);
  if (result.ok) {
    return;
  }

  if (!rocket.has<RocketStateTransitionBlocked>()) {
    instantiateNotification(world, "Rocket cannot move", result.message,
                            world.lookup("NotificationCategories::Rocket Move"),
                            NotificationSeverity::Important);
  }
  this->rocket.set<RocketStateTransitionBlocked>({.reason = result.message});
}