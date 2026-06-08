#pragma once

#include "modules/rocket/rocket_module.h"
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

/// @brief Builds a query for the active launch plans assigned to a launchpad
/// @param[in] padE The launchpad facility entity
/// @return A query matching launch plans launching from padE whose current
/// state is not terminal (i.e. excluding Launched/Cancelled). Completed plans
/// are filtered at the query level so the launchpad section never iterates over
/// historical launches.
flecs::query<LaunchPlan> activeLaunchPlansForPad(flecs::entity padE);