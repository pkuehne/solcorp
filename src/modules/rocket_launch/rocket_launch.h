#pragma once
#include <flecs.h>

// Components

/// @brief Ties together a launchpad, rocket and cargo with a date to launch by
struct LaunchPlan {
  static u_int max_id;

  u_int launch_date = 0;
  bool draft = true;
};

/// @brief Prefab for a planetary launch vehicle
struct Rocket {
  static u_int max_id;
};

struct CargoHold {
  u_int capacity = 0;
};

// Relationships
struct LaunchingFrom {}; /// From which launchpad?
struct LaunchingOn {};   /// On what  rocket
struct LaunchingWith {}; /// With what payloads?

// GUIs
struct LaunchWindow {
  flecs::entity planE;
  int launchDay = 0;
  u_int launchPrepDays = 5;

  flecs::entity rocket;
  flecs::entity launchpad;
};

// GUIs
void showLaunchWindowAdd(flecs::world, flecs::entity *rocket = nullptr,
                         flecs::entity *launchpad = nullptr);
void showLaunchWindowEdit(const flecs::entity &planE);
void hideLaunchWindow(flecs::world &world);

struct RocketLaunchModule {
  RocketLaunchModule(flecs::world &);
};
