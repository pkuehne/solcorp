#pragma once
#include <flecs.h>

struct RocketListWindow {
  flecs::entity stateFilter = flecs::entity::null();
  bool hideTerminal = true;
};

void showRocketListWindow(flecs::world &world);
void drawRocketListWindow(flecs::entity winE);

/// @brief Returns true if the rocket should appear given the current filter.
bool rocketMatchesFilter(flecs::entity rocketE, const RocketListWindow &state);
