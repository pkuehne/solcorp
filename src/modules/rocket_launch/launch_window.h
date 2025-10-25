#pragma once

#include "actions.h"
#include <flecs.h>
// #include <string>

struct LaunchWindow {
  // int launchDay = 0;

  flecs::entity planE;
  // std::string name = "";
  // flecs::entity rocket;
  // flecs::entity launchpad;
  ScheduleLaunchAction draftPlan;
};

void hideLaunchWindow(flecs::world &world);
void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket,
                         flecs::entity *launchpad);
void showLaunchWindowEdit(const flecs::entity &planE);
void systemDrawLaunchWindow(flecs::entity winE, LaunchWindow &win);
