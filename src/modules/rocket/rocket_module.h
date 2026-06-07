#pragma once
#include "modules/stats/stats.h"
#include <cstdint>
#include <flecs.h>
#include <string>

// Components

/// @brief Ties together a launchpad, rocket and cargo with a date to launch
/// by
struct LaunchPlan {
  static uint32_t max_id;

  /// When the rocket needs to start rolling out to be ready for launch
  uint32_t rollout_date = 0;
  /// When the rocket arrives on the pad (first day of on-pad prep)
  uint32_t prep_date = 0;
  /// When the launch is scheduled
  uint32_t launch_date = 0;
  /// Target orbit from CanLiftTo
  flecs::entity target_orbit = flecs::entity::null();
};

struct LaunchPlanCurrentState {};
struct LaunchPlanTargetState {};

/// @brief Component to indicate entity is a rocket.
struct Rocket {
  static uint32_t max_id;
  Stat failure_rate =
      Stat({.id = "failure-rate", .base = 0.1});
  Stat cost = Stat({.id = "cost", .base = 5'000'000});
  Stat rollout_days = Stat({.id = "rollout-days", .base = 3});
  Stat move_days =
      Stat({.id = "move-days", .base = 1});
};

struct RocketCurrentState {};
struct RocketTargetState {};
struct RocketTargetParent {};

struct RocketStateTransitionBlocked {
  std::string reason;
};

/// @brief Payload to be launched by a rocket
struct Payload {
  static uint32_t max_id;

  uint32_t mass; /// in kg
};

/// @brief Contract for a launch service. Note that the contract is not
/// directly tied to a LaunchPlan, but rather to a payload and target orbit,
/// which can then be fulfilled by any suitable LaunchPlan
struct Contract {
  static uint32_t max_id;

  std::string name;
  std::string client;
  std::string description;
  uint32_t upfront_payment;
  uint32_t completion_payment;
  bool failed = false; ///< Whether the contract was failed
};

struct ContractCurrentState {};
struct ContractTargetState {};

// Relationships
struct LaunchingFrom {}; ///< From which launchpad?
struct LaunchingOn {};   ///< On what  rocket
struct LaunchingWith {}; ///< With what payloads?

/// @brief Which orbits can this rocket lift to and with how much mass?
struct CanLiftTo {
  uint32_t max_mass; // in kg
};
struct ContractPayload {};     ///< Which contract is fullfilled by a Payload
struct ContractTargetOrbit {}; ///< Which orbit is targeted by a contract

// Systems
void systemCreateRocketPrefabs(flecs::iter &);
void systemCreateRocketBuildCategory(flecs::iter &);
void systemRocketCompleteAction(flecs::entity, Rocket &);
void systemAutoInitiateRollout(flecs::entity, LaunchPlan &);
void systemAutoCompleteRollout(flecs::entity, LaunchPlan &);
void systemAutoGoForLaunch(flecs::entity, LaunchPlan &);

struct RocketModule {
  RocketModule(flecs::world &);
};
