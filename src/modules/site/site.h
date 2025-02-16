#pragma once

#include "modules/stats/stats.h"
#include <flecs.h>
#include <vector>

/// @brief Tag which the currently displayed Site we're looking at
struct CurrentSite {};

struct Construction {
  u_int effort_remaining = 0;
  u_int effort_total = 0;
};

struct Site {
  u_int width = 10;
  u_int height = 10;
};

/// @brief Indicates that entity is a building
struct Building {};

struct SiteLocation {
  u_int x = 0;
  u_int y = 0;
};

/// @brief Allows construction of rockets
struct Manufacturing {
  std::vector<flecs::entity> lines;
  u_int max_weight = 1000;
  u_int available_effort = 50;

  Manufacturing(size_t num = 1) : lines(num) {}
};

/// @brief For rockets and payloads
struct Storage {
  u_int max_storage = 1000;
};

struct Office {
  u_int max_desks = 100;
};

/// @brief Can launch rockets
struct Launchpad {
  Stat max_weight = Stat("max-weight", "Max Weight",
                         "The maximum weight the pad can support", 1000);
};

/// @brief Indiciates the entity is a future building location
struct ConstructionSite {};

struct ConstructionSiteNeedsUpdating {};

void showBuildingWindow(const flecs::entity &buildingE);
void hideBuildingWindow(flecs::world &world);

struct SiteModule {
public:
  SiteModule(flecs::world &);
};
