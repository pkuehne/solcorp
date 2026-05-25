#pragma once
#include <flecs.h>

struct LaunchDetailWindow {
  flecs::entity plan = flecs::entity::null();
};

void showLaunchDetailWindow(flecs::entity planE);
void drawLaunchDetailWindow(flecs::entity winE);
