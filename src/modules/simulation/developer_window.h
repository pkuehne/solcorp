#pragma once

#include <flecs.h>

struct DeveloperWindow {
  bool show_metrics_window = false;
};

/// @brief Developer-only display toggles (singleton), driven from the developer
/// window's Flags tab.
struct DebugFlags {
  /// When set, build buttons show a prefab's mod-origin chain as a tooltip.
  bool prefab_provenance = false;
};

void showDeveloperWindow(flecs::world &world);
void drawDeveloperWindow(flecs::entity winE);

/**
 * @brief If the DebugFlags prefab_provenance toggle is on and the last drawn
 * ImGui item is hovered, show that prefab's mod-origin chain (PrefabProvenance)
 * as a tooltip. No-op otherwise, so it is safe to call after any build button.
 */
void drawPrefabProvenanceTooltip(const flecs::entity &prefabE);
