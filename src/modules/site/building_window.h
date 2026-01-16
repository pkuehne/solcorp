#include <flecs.h>

// GUIs
struct BuildingWindow {
  flecs::entity buildingE;
};

void showBuildingWindow(const flecs::entity &buildingE);
void drawBuildingWindow(flecs::entity winE);
