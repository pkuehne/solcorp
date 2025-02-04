#pragma once

#include <flecs.h>

struct Simulation {
  flecs::entity speed;
};

struct Game {
  u_int day = 0;
};

struct Developer {
  bool show_metrics_window = false;
};

struct SimulationModule {
  SimulationModule(flecs::world &world);
};
