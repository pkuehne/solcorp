#pragma once
#include <flecs.h>
#include <string>

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
  u_int launchPrepDays = 5;

  int launchDay = 0;

  flecs::entity planE;
  std::string name = "";
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
