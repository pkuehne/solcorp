#pragma once

#include <flecs.h>
#include <string>

struct BuildingName {
  std::string value;
};
struct BuildingPrefab {
  std::string name;
};

struct SiteLocation; // declared in site.h

flecs::entity instantiateBuilding(flecs::world &world,
                                  const BuildingName &building,
                                  const BuildingPrefab &prefab,
                                  SiteLocation location, flecs::entity site);
flecs::entity instantiateConstructionSite(flecs::world &world,
                                          SiteLocation location,
                                          flecs::entity site);
flecs::entity instantiateBuildingNotification(flecs::world &world,
                                              flecs::entity building,
                                              const std::string &text);
