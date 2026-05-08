#pragma once

#include <flecs.h>

struct MainMenuBar {};

struct EffortRequired {
  uint32_t remaining = 0;
  uint32_t total = 0;
};

struct DurationRequired {
  uint32_t remaining = 0;
  uint32_t total = 0;
};

struct MainMenuModule {
public:
  MainMenuModule(flecs::world &);
};
