#pragma once

#include "launch_actions.h"
#include <flecs.h>

struct LaunchWindow {
  LaunchScheduleAction draftPlan;
  int planningOffset = 0; ///< Extra days of buffer before rollout starts
};

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket = nullptr,
                         flecs::entity *launchpad = nullptr);
void showLaunchWindowAdd(flecs::world world, LaunchScheduleAction draftPlan);
void drawLaunchWindow(flecs::entity winE);
