#pragma once

#include "launch_actions.h"
#include <flecs.h>

struct LaunchWindow {
  LaunchScheduleAction draftPlan;
  flecs::entity editingPlan = flecs::entity::null();
  int planningOffset = 0; ///< Extra days of buffer before rollout starts
};

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket = nullptr,
                         flecs::entity *launchpad = nullptr);
void showLaunchWindowAdd(flecs::world world, LaunchScheduleAction draftPlan);
void showLaunchWindowEdit(const flecs::entity &planE);
void drawLaunchWindow(flecs::entity winE);
