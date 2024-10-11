#pragma once

#include <flecs.h>
#include <string>
#include <vector>

struct Construction {
  u_int effort_remaining = 0;
  u_int effort_total = 0;
};

struct Site {
  std::string name;
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
  u_int maxLines = 1;
  std::vector<flecs::entity> lines;
  u_int max_weight = 1000;
  u_int available_effort = 50;
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
  u_int max_weight = 1000;
};

// GUIs
struct BuildingWindow {
  flecs::entity buildingE;
};

void showBuildingWindow(const flecs::entity &buildingE);
void hideBuildingWindow(flecs::world &world);

struct SiteModule {
public:
  SiteModule(flecs::world &);
};
