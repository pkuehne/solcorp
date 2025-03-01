#pragma once

#include <flecs.h>
#include <string>

flecs::entity instantiateBuilding(flecs::world &world, const std::string &name,
                                  const std::string &prefab, u_int x, u_int y,
                                  flecs::entity site);
flecs::entity instantiateConstructionSite(flecs::world &world, u_int x, u_int y,
                                          flecs::entity site);
flecs::entity instantiateBuildingNotification(flecs::world &world,
                                              flecs::entity building,
                                              const std::string &text);
