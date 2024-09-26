#pragma once

#include <flecs.h>
#include <map>

struct KeyDown {
  int key = 0;
};

struct KeyUp {
  int key = 0;
};

struct KeyPressed {
  std::map<int, bool> keys;
};

struct InputModule {
  InputModule(flecs::world &);
};
