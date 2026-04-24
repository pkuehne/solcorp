#pragma once

#include <cstdint>
#include <flecs.h>
#include <string>

flecs::entity instantiateBuilding(flecs::world &world, const std::string &name,
                                  const std::string &prefab, uint32_t x,
                                  uint32_t y, flecs::entity site);
flecs::entity instantiateConstructionSite(flecs::world &world, uint32_t x,
                                          uint32_t y, flecs::entity site);
flecs::entity instantiateBuildingNotification(flecs::world &world,
                                              flecs::entity building,
                                              const std::string &text);
