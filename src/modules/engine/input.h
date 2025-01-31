#pragma once

#include <flecs.h>
#include <map>

// Input Events
struct KeyDown {
  int key = 0;
};

struct KeyUp {
  int key = 0;
};

struct KeyPressed {
  std::map<int, bool> keys;
};

struct MouseDown {
  int32_t x = 0;
  int32_t y = 0;
  int button = 0;
};

struct MouseUp {
  int32_t x = 0;
  int32_t y = 0;
  int button = 0;
};

void registerInput(flecs::world &);
