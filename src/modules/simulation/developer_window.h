#pragma once

#include <flecs.h>

struct DeveloperWindow {
  bool show_metrics_window = false;
};

void drawDeveloperWindow(flecs::entity winE);
