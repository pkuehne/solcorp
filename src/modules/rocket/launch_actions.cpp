#include "launch_actions.h"
#include "modules/base/action.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "modules/engine/helpers.h"
#include "modules/simulation/simulation.h"
#include "modules/site/helpers.h"
#include "modules/site/site.h"
#include "rocket_actions.h"
#include "rocket_module.h"
#include <flecs.h>
#include <format>
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

LaunchPlan LaunchScheduleAction::previewPlan() const {
  auto prepDays =
      static_cast<uint32_t>(launchpad.get<Launchpad>().prep_days.value());
  auto rollDays =
      static_cast<uint32_t>(rocket.get<Rocket>().rollout_days.value());
  auto prepDate = static_cast<uint32_t>(launchDay) - prepDays;
  return LaunchPlan{.rollout_date = prepDate - rollDays,
                    .prep_date = prepDate,
                    .launch_date = static_cast<uint32_t>(launchDay),
                    .target_orbit = targetOrbit};
}

void LaunchScheduleAction::execute(flecs::world &world) {
  auto planData = previewPlan();
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

ValidationResult LaunchCancelAction::validate(const flecs::world &) const {
  if (!plan.is_valid() || !plan.is_alive()) {
    return ValidationResult::Fail("Launch plan is not valid");
  }
  auto currentState = plan.target<LaunchPlanCurrentState>();
  if (currentState.is_valid() && currentState.has<StateIsTerminal>()) {
    return ValidationResult::Fail("Launch plan is already complete");
  }
  return ValidationResult::Pass();
}

void LaunchCancelAction::execute(flecs::world &world) {
  if (!validate(world)) {
    return;
  }

  auto rocketE = plan.target<LaunchingOn>();
  if (rocketE.is_valid()) {
    // TODO: Replace with RocketCancelMoveAction since this is a bit of a hack
    if (rocketE.has<RocketCurrentState>(
            world.lookup("States::Rocket::Moving"))) {
      rocketE.remove<RocketTargetParent>();
      rocketE.remove<RocketTargetState>(flecs::Wildcard);
      rocketE.remove<DurationRequired>();
    }
    rocketE.add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
  }

  plan.remove<LaunchingWith>(flecs::Wildcard);
  plan.remove<LaunchingOn>(flecs::Wildcard);
  plan.remove<LaunchingFrom>(flecs::Wildcard);
  plan.remove<DurationRequired>();

  spdlog::debug("Cancelling launch plan: {}", plan.name().c_str());
  plan.add<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::Cancelled"));
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

// ----- LaunchGoAction -----

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
  if (!plan.target<LaunchingOn>().is_valid()) {
    return ValidationResult::Fail("Launch plan has no rocket assigned");
  }
  uint32_t today = world.get<Game>().day;
  if (today < plan.get<LaunchPlan>().launch_date) {
    return ValidationResult::Fail("Launch date has not yet arrived");
  }
  return ValidationResult::Pass();
}

void LaunchGoAction::execute(flecs::world &world) {
  auto &planData = plan.get<LaunchPlan>();
  auto rocketE = plan.target<LaunchingOn>();
  auto launchpadE = plan.target<LaunchingFrom>();

  bool rocket_failure = roll_random(rocketE.get<Rocket>().failure_rate.value());
  spdlog::info("Rocket Launch failure {} based on chance: {}", rocket_failure,
               rocketE.get<Rocket>().failure_rate.value());

  std::vector<flecs::entity> payloads;
  plan.each<LaunchingWith>([&](flecs::entity payload) {
    if (payload.is_valid() && payload.has<Payload>()) {
      payloads.push_back(payload);
    }
  });

  auto &company = world.get_mut<Company>();
  uint32_t total_payment = 0;

  for (auto payload : payloads) {
    if (payload.is_valid() && payload.has<Payload>()) {
      spdlog::debug("Removing payload: {}", payload.name().c_str());
      auto contractE = payload.target<ContractPayload>();
      SC_ASSERT(contractE.is_valid() && contractE.has<Contract>(),
                "Payload {} has ContractPayload relationship to invalid or "
                "non-contract entity");

      auto &contract = contractE.get_mut<Contract>();
      if (contractE.target<ContractTargetOrbit>() != planData.target_orbit) {
        spdlog::info(
            "Contract {} failed because payload {} was launched to wrong orbit",
            contractE.name().c_str(), payload.name().c_str());
        contract.failed = true;
      } else if (rocket_failure) {
        spdlog::info(
            "Contract {} failed because payload {} was launched on a rocket "
            "that failed",
            contractE.name().c_str(), payload.name().c_str());
        contract.failed = true;
        instantiateNotification(
            world, "Launch Failure",
            fmt::format("{} was launched on {}, which failed."
                        " Contract {} is failed.",
                        payload.name().c_str(), rocketE.name().c_str(),
                        contractE.name().c_str()));
      } else {
        contract.failed = false;
        company.balance += static_cast<int64_t>(contract.completion_payment);
        total_payment += contract.completion_payment;
        instantiateNotification(
            world, "Payload Launched",
            fmt::format("{} was launched on {} to {}", payload.name().c_str(),
                        rocketE.name().c_str(),
                        planData.target_orbit.name().c_str()),
            world.lookup("NotificationCategories::Rocket Launch"));
      }
      contract.status = ContractStatus::Closed;
    }
  }

  spdlog::debug("Removing plan: {} launch_date: {} today: {}", plan.id(),
                planData.launch_date, world.get<Game>().day);

  std::string notification;
  if (rocket_failure) {
    notification = std::format("{} failed - {} exploded on launch",
                               plan.name().c_str(), rocketE.name().c_str());
  } else {
    notification = std::format("{} launched {} successfully ({})",
                               plan.name().c_str(), rocketE.name().c_str(),
                               ("$" + format_locale(total_payment)).c_str());
  }
  notification +=
      fmt::format("\nOrbit:     {}", planData.target_orbit.name().c_str());
  notification += fmt::format("\nLaunchpad: {}", launchpadE.name().c_str());
  notification += fmt::format("\nRocket:    {}", rocketE.name().c_str());
  std::string payload_list;
  for (auto payload : payloads) {
    payload_list += "\n- " + std::string(payload.name().c_str());
  }
  notification += "\nPayloads:" + payload_list;

  instantiateBuildingNotification(world, launchpadE, notification);
  instantiateNotification(world, "Launch Complete", notification,
                          world.lookup("NotificationCategories::Rocket Launch"),
                          rocket_failure ? NotificationSeverity::Critical
                                         : NotificationSeverity::High);

  for (auto payload : payloads) {
    payload.destruct();
  }
  rocketE.destruct();
  plan.add<LaunchPlanCurrentState>(
      world.lookup("States::LaunchPlan::Launched"));
}
