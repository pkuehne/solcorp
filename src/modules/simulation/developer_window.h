#pragma once

#include <flecs.h>

struct DeveloperWindow {
  bool open = true;
};

void showDeveloperWindow(flecs::world &world);
void hideDeveloperWindow(flecs::world &world);
void systemDrawDeveloperWindow(flecs::entity winE, DeveloperWindow &win);
