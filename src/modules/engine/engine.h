#pragma once
#include <flecs.h>
#include <map>

// Phases
extern flecs::entity PreFramePhase;
extern flecs::entity ValidatePhase;
extern flecs::entity PostValidatePhase;
extern flecs::entity UpdatePhase;
extern flecs::entity GuiPhase;
extern flecs::entity PreRenderPhase;
extern flecs::entity RenderPhase;
extern flecs::entity PostRenderPhase;
extern flecs::entity PostFramePhase;

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

// Engine Module Entry Point
struct EngineModule {
  EngineModule(flecs::world &);
};
