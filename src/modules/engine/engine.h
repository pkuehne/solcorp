#pragma once
#include <flecs.h>

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

// Engine Module Entry Point
struct EngineModule {
  EngineModule(flecs::world &);
};
