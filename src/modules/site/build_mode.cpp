#include "build_mode.h"
#include "modules/base/base.h"
#include <spdlog/spdlog.h>

#include <format>
#include <string>

void enterBuildMode(flecs::world &world, flecs::entity prefab) {
  if (!prefab.is_alive()) {
    spdlog::error("Cannot enter build mode for an invalid prefab");
    return;
  }
  world.set<BuildMode>({});
  world.entity<BuildMode>().add<BuildModePrefab>(prefab);
}

void exitBuildMode(flecs::world &world) {
  world.remove<BuildMode>();
  world.entity<BuildMode>().remove<BuildModePrefab>(flecs::Wildcard);
}

PendingPlacement pendingPlacement(flecs::entity prefab, const BuildMode &mode,
                                  SiteLocation hover, const Site &site) {
  const FootprintLimits limits = placementLimits(prefab);
  if (prefab.has<FixedSize>() || !mode.dragging) {
    return {.origin = hover, .footprint = limits.min};
  }
  return dragRectangle(mode.drag_start, hover, limits, site);
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

  if (prefab.has<FixedSize>()) {
    tooltip.hint = "Click to build \u00b7 Right-click to cancel";
    return tooltip;
  }

  const FootprintLimits limits = placementLimits(prefab);
  if (limits.max.w != UINT8_MAX || limits.max.h != UINT8_MAX) {
    tooltip.size += " (max " + sizeText(limits.max) + ")";
  }
  tooltip.hint = mode.dragging
                     ? "Release to build \u00b7 Right-click to cancel"
                     : "Click and drag to size \u00b7 Right-click to cancel";
  return tooltip;
}

flecs::entity buildModePrefab(const flecs::world &world) {
  if (!world.has<BuildMode>()) {
    return {};
  }
  return world.entity<BuildMode>().target<BuildModePrefab>();
}
