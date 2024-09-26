#pragma once

#include <flecs.h>

struct GameResource {
  flecs::entity sim_speed;
  u_int day = 0;
};
