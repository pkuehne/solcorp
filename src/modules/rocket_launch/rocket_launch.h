#pragma once
#include <flecs.h>
#include <string>

// Components

/// @brief Ties together a launchpad, rocket and cargo with a date to launch
/// by
struct LaunchPlan {
  static u_int max_id;

  u_int launch_date = 0;
  flecs::entity target_orbit =
      flecs::entity::null(); ///< Target orbit from CanLiftTo
};

/// @brief Prefab for a planetary launch vehicle
struct Rocket {
  static u_int max_id;
};

/// @brief Payload to be launched by a rocket
struct Payload {
  static u_int max_id;

  u_int mass; /// in kg
};

enum class ContractStatus : uint8_t { Open, Accepted, Closed };

/// @brief Contract for a launch service. Note that the contract is not
/// directly tied to a LaunchPlan, but rather to a payload and target orbit,
/// which can then be fulfilled by any suitable LaunchPlan
struct Contract {
  static u_int max_id;

  std::string client;
  std::string description;
  float upfront_payment;
  float completion_payment;
  ContractStatus status = ContractStatus::Open;
  bool failed = false; ///< Whether the contract was failed
};

// Relationships
struct LaunchingFrom {}; ///< From which launchpad?
struct LaunchingOn {};   ///< On what  rocket
struct LaunchingWith {}; ///< With what payloads?
/// @brief Which orbits can this rocket lift to and with how much mass?
struct CanLiftTo {
  u_int max_mass; // in kg
};
struct ContractPayload {};     ///< Which contract is fullfilled by a Payload
struct ContractTargetOrbit {}; ///< Which orbit is targeted by a contract

// Systems
void systemLaunchRocket(flecs::entity, LaunchPlan &);
void systemCreateRocketPrefabs(flecs::iter &);

struct RocketLaunchModule {
  RocketLaunchModule(flecs::world &);
};
