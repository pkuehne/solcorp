#include <flecs.h>

struct ConstructionSiteWindow {
  flecs::entity buildingE;
};

void showConstructionSiteWindow(const flecs::entity &entity);
void hideConstructionSiteWindow(flecs::world &world);
void systemDrawConstructionSiteWindow(flecs::entity winE,
                                      ConstructionSiteWindow &win);
