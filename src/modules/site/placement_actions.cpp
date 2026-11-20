#include "placement_actions.h"
#include "connector.h"
#include "modules/base/base.h"
#include "modules/engine/render.h"
#include <spdlog/spdlog.h>

#include <format>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string displayName(flecs::entity prefab) {
  return prefab.has<Label>() ? prefab.get<Label>().label
                             : std::string{prefab.name().c_str()};
}

std::vector<std::string> uniqueNames(PlacedPrefab prefab, TargetSite site,
                                     size_t count) {
  const std::string base = displayName(prefab.value);
  std::vector<std::string> names;
  int index = 1;
  while (names.size() < count) {
    auto name = std::format("{} {}", base, index++);
    if (!site.value.lookup(name.c_str()).is_valid()) {
      names.push_back(std::move(name));
    }
  }
  return names;
}

Transform transformAt(SiteLocation location) {
  Transform transform;
  transform.relativePosition = {
      .x = static_cast<float>(location.x * SITE_CELL_SIZE),
      .y = static_cast<float>(location.y * SITE_CELL_SIZE)};
  return transform;
}

} // namespace

ValidationResult PlaceConnectorAction::validate(const flecs::world &) const {
  if (!prefab.is_alive() || !prefab.has<ConnectorTile>()) {
    return ValidationResult::Fail("No road is selected");
  }
  if (!site.is_alive() || !site.has<Site>()) {
    return ValidationResult::Fail("Site is not valid");
  }
  if (footprint.w != 1 && footprint.h != 1) {
    return ValidationResult::Fail("Roads are one cell wide");
  }
  return ValidationResult::Issue(placementMessage(canPlace(
      buildOccupancyGrid(site), PlacementKind::Connector, origin, footprint)));
}

void PlaceConnectorAction::execute(flecs::world &world) {
  if (auto result = validate(world); !result) {
    spdlog::warn("Cannot place road: {}", result.message);
    return;
  }
  const auto count = static_cast<size_t>(footprint.w) * footprint.h;
  const auto names = uniqueNames(PlacedPrefab{prefab}, TargetSite{site}, count);
  size_t next = 0;
  for (int dy = 0; std::cmp_less(dy, footprint.h); ++dy) {
    for (int dx = 0; std::cmp_less(dx, footprint.w); ++dx) {
      const SiteLocation cell{.x = static_cast<uint8_t>(origin.x + dx),
                              .y = static_cast<uint8_t>(origin.y + dy)};
      world.entity(names[next++].c_str())
          .is_a(prefab)
          .set<SiteLocation>(cell)
          .set<Transform>(transformAt(cell))
          .child_of(site);
    }
  }
  site.add<SiteNeedsRelayout>();
}

ValidationResult PlaceBuildingAction::validate(const flecs::world &) const {
  if (!prefab.is_alive()) {
    return ValidationResult::Fail("No building is selected");
  }
  if (!site.is_alive() || !site.has<Site>()) {
    return ValidationResult::Fail("Site is not valid");
  }
  if (prefab.has<FixedSize>()) {
    if (footprint.w != 1 || footprint.h != 1) {
      return ValidationResult::Fail("This building only comes in one size");
    }
  } else {
    const auto *limits = prefab.try_get<FootprintLimits>();
    const auto size = checkFootprintSize(
        footprint, limits != nullptr ? *limits : FootprintLimits{});
    if (size != PlacementResult::Ok) {
      return ValidationResult::Fail(placementMessage(size));
    }
  }
  return ValidationResult::Issue(placementMessage(canPlace(
      buildOccupancyGrid(site), PlacementKind::Building, origin, footprint)));
}

void PlaceBuildingAction::execute(flecs::world &world) {
  if (auto result = validate(world); !result) {
    spdlog::warn("Cannot place building: {}", result.message);
    return;
  }

  world
      .entity(uniqueNames(PlacedPrefab{prefab}, TargetSite{site}, 1)[0].c_str())
      .is_a(prefab)
      .set<SiteLocation>(origin)
      .set<Footprint>(footprint)
      .set<Transform>(transformAt(origin))
      .child_of(site);
  site.add<SiteNeedsRelayout>();
}

std::unique_ptr<IAction>
makePlacementAction(PlacedPrefab prefab, TargetSite site, PlacementKind kind,
                    const PendingPlacement &placement) {
  if (kind == PlacementKind::Connector) {
    return std::make_unique<PlaceConnectorAction>(
        prefab, site, placement.origin, placement.footprint);
  }
  return std::make_unique<PlaceBuildingAction>(prefab, site, placement.origin,
                                               placement.footprint);
}
