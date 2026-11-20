#pragma once

#include "../base/action.h"
#include "placement.h"
#include "site.h"
#include <flecs.h>
#include <memory>

struct PlacedPrefab {
  flecs::entity value;
};
struct TargetSite {
  flecs::entity value;
};

struct PlaceConnectorAction : public IAction {
  flecs::entity prefab;
  flecs::entity site;
  SiteLocation origin;
  Footprint footprint;

  PlaceConnectorAction(PlacedPrefab p, TargetSite s, SiteLocation o,
                       Footprint f)
      : prefab(p.value), site(s.value), origin(o), footprint(f) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};

struct PlaceBuildingAction : public IAction {
  flecs::entity prefab;
  flecs::entity site;
  SiteLocation origin;
  Footprint footprint;

  PlaceBuildingAction(PlacedPrefab p, TargetSite s, SiteLocation o, Footprint f)
      : prefab(p.value), site(s.value), origin(o), footprint(f) {}

  [[nodiscard]] ValidationResult
  validate(const flecs::world &world) const override;
  void execute(flecs::world &world) override;
};

/// @brief The action that places @p placement of @p prefab, by its kind
std::unique_ptr<IAction> makePlacementAction(PlacedPrefab prefab,
                                             TargetSite site,
                                             PlacementKind kind,
                                             const PendingPlacement &placement);
