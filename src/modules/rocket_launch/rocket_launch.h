#pragma once
#include <flecs.h>

// Components

/// @brief Ties together a launchpad, rocket and cargo with a date to launch
/// by
struct LaunchPlan {
  static u_int max_id;

  u_int launch_date = 0;
};

/// @brief Prefab for a planetary launch vehicle
struct Rocket {
  static u_int max_id;
};

struct Payload {
  static u_int max_id;

  u_int mass; /// in kg
};

// Relationships
struct LaunchingFrom {}; /// From which launchpad?
struct LaunchingOn {};   /// On what  rocket
struct LaunchingWith {}; /// With what payloads?
struct CanLiftTo {
  u_int max_mass; // in kg
}; /// To which orbit can this rocket lift and how much mass?

// Systems
void systemLaunchRocket(flecs::entity, LaunchPlan &);
void systemCreateRocketPrefabs(flecs::iter &);

struct RocketLaunchModule {
  RocketLaunchModule(flecs::world &);
};
