#pragma once

#include <flecs.h>

struct RocketPrefabWindow {
  flecs::entity manufacturingE;
};

void showRocketPrefabWindow(const flecs::entity &entity);
void drawRocketPrefabWindow(flecs::entity winE);
