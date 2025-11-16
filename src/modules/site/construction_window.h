#include <flecs.h>

struct ConstructionSiteWindow {
  flecs::entity buildingE;
};

void showConstructionSiteWindow(const flecs::entity &entity);
void drawConstructionSiteWindow(flecs::entity winE);
