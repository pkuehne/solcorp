#pragma once

#include "placement.h"
#include "site.h"
#include <flecs.h>

/// @brief The active build tool; absent when the player is not building
struct BuildMode {
  PlacementKind kind = PlacementKind::Building;
  SiteLocation drag_start{};
  bool dragging = false;
};

/// @brief BuildMode --BuildModePrefab--> prefab being placed (exclusive)
struct BuildModePrefab {};

void enterBuildMode(flecs::world &world, flecs::entity prefab);

void exitBuildMode(flecs::world &world);

/// @brief What a release at @p hover would place with @p prefab: one cell if
/// FixedSize, the rectangle dragged from mode.drag_start while dragging,
/// otherwise the minimum footprint at @p hover
PendingPlacement pendingPlacement(flecs::entity prefab, const BuildMode &mode,
                                  SiteLocation hover, const Site &site);

/// @brief Invalid when not building
flecs::entity buildModePrefab(const flecs::world &world);
