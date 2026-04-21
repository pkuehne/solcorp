#pragma once

#include "modules/stats/stats.h"
#include <cstdint>
#include <flecs.h>

/// @brief Tag which the currently displayed Site we're looking at
struct CurrentSite {};

struct Construction {
  uint32_t effort_remaining = 0;
  uint32_t effort_total = 0;
};

struct Site {
  uint32_t width = 10;
  uint32_t height = 10;
};

/// @brief Indicates that entity is a building
struct Building {};

struct SiteLocation {
  uint32_t x = 0;
  uint32_t y = 0;
};

/// @brief Marker component for facilities within a building
struct Facility {};

/// @brief Facility for constructing rockets
struct Manufacturing {
  uint32_t max_weight = 1000;
  uint32_t available_effort = 50;
};

/// @brief For rockets and payloads
struct Storage {
  uint32_t max_storage = 1000;
};

struct Office {
  Stat max_desks =
      Stat("max-desks", "Max Desks",
           "The maximum number of desks this facility can hold", 100);
};

/// @brief Can launch rockets
struct Launchpad {
  Stat max_weight = Stat("max-weight", "Max Weight",
                         "The maximum weight the pad can support", 1000);
  Stat prep_days =
      Stat("prep-days", "Prep Days",
           "Number of days required to prepare a launch", 5, false);
};

/// @brief Indiciates the entity is a future building location
struct ConstructionSite {};

struct ConstructionSiteNeedsUpdating {};

void systemCreateSitePrefabs(flecs::iter &);
void systemCreateSiteWindows(flecs::iter &it);
void systemBuildingUpdateManufacuringProgress(flecs::entity, Manufacturing &);

struct SiteModule {
public:
  SiteModule(flecs::world &);
};
