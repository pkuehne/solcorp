#pragma once

#include <flecs.h>
#include <vector>

class SiteWindow {
public:
  void draw(flecs::world &world);
  void loadData();
  void drawRocket(flecs::entity &rocket);
  void drawStorageSection();
  void drawManufacturingSection();

  void movePopup(const flecs::entity &source, flecs::entity &rocket);
  void schedulePopup(flecs::entity &rocket);

  bool visible = true;
  flecs::entity siteEntity;

private:
  std::vector<flecs::entity> m_manuBuildings;
  std::vector<flecs::entity> m_storageBuildings;
};
