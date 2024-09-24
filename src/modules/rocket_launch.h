#pragma once
#include <flecs.h>
#include <string>

// Components

/// @brief Ties together a launchpad, rocket and cargo with a date to launch by
struct LaunchPlan {
  u_int launch_date = 0;
  bool draft = true;
};

/// @brief Identifies a planetary launch vehicle
struct Rocket {};

struct RocketPrefab {};

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
  std::string rocketDisplay;

  flecs::entity launchpad;
  std::string launchpadDisplay;
};

// GUIs
void showLaunchWindow(const flecs::entity &planE);
void hideLaunchWindow(flecs::world &world);

struct RocketLaunchModule {
  RocketLaunchModule(flecs::world &);
};
