#include <flecs.h>

// GUIs
struct BuildingWindow {
  flecs::entity buildingE;
  bool open = true;
};

void systemDrawBuildingWindow(flecs::entity winE, BuildingWindow &win);
