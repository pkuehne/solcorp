#pragma once

#include <flecs.h>

struct ActiveLaunchesWindow {
  flecs::entity filterSite    = flecs::entity::null();
  flecs::entity filterPad     = flecs::entity::null();
  flecs::entity filterOrbit   = flecs::entity::null();
  flecs::entity pendingCancel = flecs::entity::null();
};

void showActiveLaunchesWindow(flecs::world &world);
void drawActiveLaunchesWindow(flecs::entity winE);

/// @brief Returns true if planE passes all active filters in state.
/// Dead filter entities (destructed after being set) are treated as no-filter.
/// Exposed for testing.
bool planMatchesFilters(flecs::entity planE, const ActiveLaunchesWindow &state);
