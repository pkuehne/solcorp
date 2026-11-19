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

/// @brief The mouse moved this frame; @p over_ui is set when ImGui has it
struct MouseMove {
  int32_t x = 0;
  int32_t y = 0;
  bool over_ui = false;
};

/// @brief Size of the window's drawable area in renderer pixels; set at
/// startup and whenever the window is resized
struct WindowSize {
  int width = 0;
  int height = 0;
};

void registerInput(flecs::world &);
