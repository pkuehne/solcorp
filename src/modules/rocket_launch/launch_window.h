#pragma once

#include "actions.h"
#include <flecs.h>

struct LaunchWindow {
  ScheduleLaunchAction draftPlan;
};

void showLaunchWindowAdd(flecs::world world, flecs::entity *rocket = nullptr,
                         flecs::entity *launchpad = nullptr);
void showLaunchWindowEdit(const flecs::entity &planE);
void drawLaunchWindow(flecs::entity winE);
