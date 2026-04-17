#pragma once
#include <flecs.h>

void registerEngineComponents(flecs::world &);

// Engine Module Entry Point
struct EngineModule {
  EngineModule(flecs::world &);
};
