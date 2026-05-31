#pragma once

#include <flecs.h>

struct BuildingDetailWindow {
  flecs::entity buildingE;
};

void showBuildingDetailWindow(const flecs::entity &buildingE);
void drawBuildingDetailWindow(flecs::entity winE);

/// @brief Gets the progress of the current effort on an entity, if it has one
/// @param[in] entity The entity to check for an EffortRequired component
/// @return A float between 0 and 1 representing the progress of the effort, or
/// 1 if there is no EffortRequired component
float getEntityEffortRequired(flecs::entity &entity);