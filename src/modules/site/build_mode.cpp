#include "build_mode.h"
#include "connector.h"
#include "modules/base/base.h"
#include <spdlog/spdlog.h>

#include <format>
#include <string>

void enterBuildMode(flecs::world &world, flecs::entity prefab) {
  if (!prefab.is_alive()) {
    spdlog::error("Cannot enter build mode for an invalid prefab");
    return;
  }
  world.set<BuildMode>({.kind = prefab.has<ConnectorTile>()
                                    ? PlacementKind::Connector
                                    : PlacementKind::Building,
                        .drag_start = {},
                        .dragging = false});
  world.entity<BuildMode>().add<BuildModePrefab>(prefab);
}

void exitBuildMode(flecs::world &world) {
  world.remove<BuildMode>();
  world.entity<BuildMode>().remove<BuildModePrefab>(flecs::Wildcard);
}

PendingPlacement pendingPlacement(flecs::entity prefab, const BuildMode &mode,
                                  SiteLocation hover, const Site &site) {
  const FootprintLimits limits = placementLimits(prefab);
  if (!mode.dragging) {
    return {.origin = hover, .footprint = limits.min};
  }
  switch (dragShape(prefab)) {
  case DragShape::Rectangle:
    return dragRectangle(mode.drag_start, hover, limits, site);
  case DragShape::Run:
    return dragRun(mode.drag_start, hover);
  case DragShape::None:
    break;
  }
  return {.origin = hover, .footprint = limits.min};
}

namespace {

std::string sizeText(Footprint size) {
  auto dimension = [](uint8_t value) {
    return value == UINT8_MAX ? std::string{"any"} : std::to_string(value);
  };
  return std::format("{} \u00d7 {}", dimension(size.w), dimension(size.h));
}

} // namespace

GhostTooltip ghostTooltip(flecs::entity prefab, const BuildMode &mode,
                          const PendingPlacement &placement,
                          const ValidationResult &validation) {
  GhostTooltip tooltip;
  tooltip.title = prefab.has<Label>() ? prefab.get<Label>().label
                                      : std::string{prefab.name().c_str()};
  tooltip.size = "Size " + sizeText(placement.footprint);
  tooltip.problem = validation.message;

  const DragShape shape = dragShape(prefab);
  if (shape == DragShape::Rectangle) {
    const FootprintLimits limits = placementLimits(prefab);
    if (limits.max.w != UINT8_MAX || limits.max.h != UINT8_MAX) {
      tooltip.size += " (max " + sizeText(limits.max) + ")";
    }
  }

  std::string action;
  if (mode.dragging && shape != DragShape::None) {
    action = "Release to build";
  } else if (shape == DragShape::Rectangle) {
    action = "Click and drag to size";
  } else if (shape == DragShape::Run) {
    action = "Click and drag to lay road";
  } else {
    action = "Click to build";
  }
  tooltip.hint = action + " \u00b7 Right-click to cancel";
  return tooltip;
}

flecs::entity buildModePrefab(const flecs::world &world) {
  if (!world.has<BuildMode>()) {
    return {};
  }
  return world.entity<BuildMode>().target<BuildModePrefab>();
}
