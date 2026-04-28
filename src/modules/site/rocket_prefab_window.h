#pragma once

#include <cstdint>
#include <flecs.h>
#include <vector>

struct Company;

struct RocketPrefabWindow {
  flecs::entity manufacturingE;
};

void showRocketPrefabWindow(const flecs::entity &entity);
void drawRocketPrefabWindow(flecs::entity winE);

std::vector<flecs::entity> collectRocketPrefabs(flecs::world &world);
int64_t computeRocketPrefabBuildCost(const flecs::entity &prefabE);
bool canAffordRocketPrefab(const Company &company,
                           const flecs::entity &prefabE);
