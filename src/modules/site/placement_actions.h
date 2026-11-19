#pragma once

#include "../base/action.h"
#include "placement.h"
#include "site.h"
#include <flecs.h>

struct PlacedPrefab {
  flecs::entity value;
};
struct TargetSite {
  flecs::entity value;
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
