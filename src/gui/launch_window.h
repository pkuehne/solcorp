#pragma once

#include <flecs.h>

class LaunchWindow {
public:
  void draw(flecs::world &world);
  void loadData();

  flecs::entity launchPlanEntity;
  bool visible = false;
};
