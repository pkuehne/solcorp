#include "placement_actions.h"
#include "connector.h"
#include "modules/base/base.h"
#include "modules/engine/render.h"
#include <spdlog/spdlog.h>

#include <format>
#include <string>

namespace {

std::string displayName(flecs::entity prefab) {
  return prefab.has<Label>() ? prefab.get<Label>().label
                             : std::string{prefab.name().c_str()};
}

std::string uniqueName(PlacedPrefab prefab, TargetSite site) {
  const std::string base = displayName(prefab.value);
  std::string name;
  int index = 1;
  do {
    name = std::format("{} {}", base, index++);
  } while (site.value.lookup(name.c_str()).is_valid());
  return name;
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
  return ValidationResult::Issue(placementMessage(canPlace(
      buildOccupancyGrid(site), PlacementKind::Connector, location, {})));
}

void PlaceConnectorAction::execute(flecs::world &world) {
  if (auto result = validate(world); !result) {
    spdlog::warn("Cannot place road: {}", result.message);
    return;
  }
  world.entity(uniqueName(PlacedPrefab{prefab}, TargetSite{site}).c_str())
      .is_a(prefab)
      .set<SiteLocation>(location)
      .set<Transform>(transformAt(location))
      .child_of(site);
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

  world.entity(uniqueName(PlacedPrefab{prefab}, TargetSite{site}).c_str())
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
    return std::make_unique<PlaceConnectorAction>(prefab, site,
                                                  placement.origin);
  }
  return std::make_unique<PlaceBuildingAction>(prefab, site, placement.origin,
                                               placement.footprint);
}
