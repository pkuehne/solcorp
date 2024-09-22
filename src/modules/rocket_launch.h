#pragma once
#include <flecs.h>

// Components
struct LaunchPlan {
  u_int launch_date = 0;
  bool draft = true;
};

// Relationships
struct LaunchingFrom {}; /// From which launchpad?
struct LaunchingOn {};   /// On what  rocket
struct LaunchingWith {}; /// With what payloads?

// Systems
void systemLaunchRocket(flecs::entity, LaunchPlan &);

struct RocketLaunchModule {

  RocketLaunchModule(flecs::world &);
};
