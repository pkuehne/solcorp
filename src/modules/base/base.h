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

/// @brief Returns the nearest ancestor of e (inclusive) that has component T,
/// or a null entity if none is found.
template <typename T> flecs::entity findAncestorWith(flecs::entity e) {
  for (auto it = e; it.is_valid(); it = it.parent()) {
    if (it.has<T>()) {
      return it;
    }
  }
  return flecs::entity::null();
}

struct Simulation {
  flecs::entity speed;
};

struct Game {
  u_int day = 0;
};

struct BaseModule {
  BaseModule(flecs::world &world);
};
