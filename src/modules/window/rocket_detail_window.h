#pragma once
#include <flecs.h>

struct RocketDetailWindow {
  flecs::entity rocket = flecs::entity::null();
};

void showRocketDetailWindow(flecs::entity rocketE);
void drawRocketDetailWindow(flecs::entity winE);
void drawRocketButtons(flecs::entity &rocket);
