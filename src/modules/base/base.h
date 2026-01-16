#pragma once

#include <flecs.h>

// Phases
extern flecs::entity PostStartPhase;
extern flecs::entity PreFramePhase;
extern flecs::entity ValidatePhase;
extern flecs::entity PostValidatePhase;
extern flecs::entity UpdatePhase;
extern flecs::entity GuiPhase;
extern flecs::entity PreRenderPhase;
extern flecs::entity RenderPhase;
extern flecs::entity PostRenderPhase;
extern flecs::entity PostFramePhase;

constexpr double pi = 3.14159265358979323846;
constexpr double deg2rad(double deg) { return deg * pi / 180.0; }

struct BaseModule {
  BaseModule(flecs::world &world);
};
