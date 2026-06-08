#pragma once

#include "modules/rocket/launch_actions.h"
#include <flecs.h>
#include <vector>

struct LaunchWindowDraftOptions {
  flecs::entity rocket = flecs::entity::null();
  flecs::entity launchpad = flecs::entity::null();
  flecs::entity targetOrbit = flecs::entity::null();
  std::vector<flecs::entity> payloads;
};

struct LaunchWindow {
  LaunchScheduleAction draftPlan;
  int planningOffset = 0; ///< Extra days of buffer before rollout starts
};

LaunchScheduleAction
buildLaunchScheduleDraft(const LaunchWindowDraftOptions &options);
void showLaunchWindowAdd(
    flecs::world world,
    const LaunchWindowDraftOptions &options = LaunchWindowDraftOptions{});
void drawLaunchWindow(flecs::entity winE);
