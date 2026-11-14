#pragma once

#include "placement.h"
#include "site.h"
#include <flecs.h>

/// @brief The active build tool, as a singleton; absent when the player is
/// not building (ADR 009 §3)
struct BuildMode {
  PlacementKind kind = PlacementKind::Building;
  SiteLocation drag_start{};
  bool dragging = false;
};

/// @brief The prefab being placed.
/// BuildMode --BuildModePrefab--> prefab (exclusive, ADR 014)
struct BuildModePrefab {};

/// @brief Starts placing @p prefab, replacing any prefab already selected
void enterBuildMode(flecs::world &world, flecs::entity prefab);

/// @brief Leaves build mode; clicks open building windows again
void exitBuildMode(flecs::world &world);

/// @brief The prefab being placed, or an invalid entity when not building
flecs::entity buildModePrefab(const flecs::world &world);
