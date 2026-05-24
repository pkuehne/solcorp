#include "rocket_actions.h"
#include "modules/base/action.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "modules/simulation/simulation.h"
#include "rocket_module.h"
#include <flecs.h>
#include <format>
#include <spdlog/spdlog.h>

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

  auto rocket =
      world.entity()
          .is_a(this->prefab)
          .add<RocketCurrentState>(
              world.lookup("States::Rocket::UnderConstruction"))
          .add<RocketTargetState>(world.lookup("States::Rocket::Stored"))
          .set<EffortRequired>({.remaining = 300, .total = 300})
          .child_of(this->line);
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
  if (!rocket.has<RocketCurrentState>(
          rocket.world().lookup("States::Rocket::UnderConstruction"))) {
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

  rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
  rocket.remove<RocketTargetState>(flecs::Wildcard);
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
  if (!rocket.has<RocketCurrentState>(
          rocket.world().lookup("States::Rocket::Stored")) &&
      !rocket.has<RocketCurrentState>(
          rocket.world().lookup("States::Rocket::Assigned"))) {
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
  rocket.add<RocketCurrentState>(world.lookup("States::Rocket::Moving"));
  rocket.add<RocketTargetState>(world.lookup("States::Rocket::Stored"));
  rocket.set<DurationRequired>({.remaining = days, .total = days});
}

// ----- RocketMoveCompleteAction-----

ValidationResult
RocketCompleteMoveAction::validate(const flecs::world &) const {
  if (!rocket.is_valid()) {
    return ValidationResult::Fail("Rocket is not valid");
  }
  if (!rocket.has<RocketCurrentState>(
          rocket.world().lookup("States::Rocket::Moving"))) {
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
  rocket.add<RocketCurrentState>(rocket.target<RocketTargetState>());
  rocket.remove<RocketTargetState>(flecs::Wildcard);
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
