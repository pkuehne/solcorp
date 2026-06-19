#pragma once

#include "modules/base/base.h"
#include "modules/stats/stats.h"
#include <cstdint>
#include <flecs.h>

struct Rocket;

/// @brief Tag which the currently displayed Site we're looking at
struct CurrentSite {};

struct Site {
  uint8_t width = 10;
  uint8_t height = 10;
};

/// @brief Indicates that entity is a building
struct Building {};

struct SiteLocation {
  uint8_t x = 0;
  uint8_t y = 0;
};

/// @brief Marker component for facilities within a building
struct Facility {};

/// @brief Facility for constructing rockets
struct Manufacturing {
  uint32_t max_weight = 1000;
  uint32_t available_effort = 50;
  bool auto_build_next = false;
  bool auto_store = false;
};

/// @brief Which rocket prefab is being built here, if any
struct ManufacturingLineTemplate {};
/// @brief Which storage completed rockets are being stored in, if any
struct ManufacturingLineStorage {};
/// @brief Set when auto_build_next fails validation; cleared when a build
/// starts again
struct AutoBuildBlocked {};
/// @brief Set when auto_store fails validation; cleared when the validation
/// later succeeds
struct AutoStoreBlocked {};

/// @brief For rockets and payloads
struct Storage {
  uint32_t max_storage = 1000;
};

struct Office {
  Stat max_desks = Stat({.id = "max-desks", .base = 100});
};

/// @brief Can launch rockets
struct Launchpad {
  Stat max_weight = Stat({.id = "max-weight", .base = 1000});
  Stat prep_days = Stat({.id = "prep-days", .base = 5});
};

/// @brief Indiciates the entity is a future building location
struct ConstructionSite {};

/// @brief Tag on a Site meaning its layout changed and derived state (e.g.
/// connector autotiling) must be recomputed. Consumed in ValidatePhase.
struct SiteNeedsRelayout {};

void systemCreateSitePrefabs(flecs::iter &);
void systemCreateSiteWindows(flecs::iter &it);
void systemBuildingUpdateManufacuringProgress(flecs::entity, EffortRequired &,
                                              const Manufacturing &);
void systemAutoStartNextBuild(flecs::entity manufacturingE,
                              const Manufacturing &);
void systemAutoStoreBuiltRocket(flecs::entity rocketE, Rocket &,
                                const Manufacturing &);

struct SiteModule {
public:
  SiteModule(flecs::world &);
};
