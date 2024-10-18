#include <flecs.h>

// GUIs
struct BuildingWindow {
  flecs::entity buildingE;
};

void systemDrawBuildingWindow(flecs::entity winE, BuildingWindow &win);
