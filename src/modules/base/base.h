#pragma once

#include <cstdint>
#include <flecs.h>
#include <numbers>
#include <string>

/// Tag placed on state entities that are terminal (no further transitions).
/// Queries use this to distinguish active from completed/archived records.
struct StateIsTerminal {};

/// User-visible display name for any entity. Entities without this component
/// fall back to their ECS name in UI; entities intended as internal/debug only
/// should omit it.
struct Label {
  std::string label;
};

struct EffortRequired {
  uint32_t remaining = 0;
  uint32_t total = 0;
};

struct DurationRequired {
  uint32_t remaining = 0;
  uint32_t total = 0;
};

struct Config {
  std::string font = "Roboto-Medium.ttf";
  uint32_t font_size = 16;
};

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

consteval double deg2rad(double deg) { return deg * std::numbers::pi / 180.0; }

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
  uint32_t day = 0;
};

struct BaseModule {
  BaseModule(flecs::world &world);
};
