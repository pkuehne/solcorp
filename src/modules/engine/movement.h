#pragma once

#include <flecs.h>

struct Velocity {
  float x = 0;
  float y = 0;
};

struct Expire {
  int millis = 0;
};

void registerMovement(flecs::world &);
