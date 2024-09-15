#pragma once

#include <flecs.h>
#include <vector>

class SiteWindow {
public:
  void show(const flecs::entity &site);
  void hide();

  void draw(flecs::world &world);

private:
  void loadData();
  void drawRocket(flecs::entity &rocket);
  void drawStorageSection();
  void drawManufacturingSection();

  void movePopup(const flecs::entity &source, flecs::entity &rocket);
  void schedulePopup(flecs::entity &rocket);

private:
  bool m_visible = true;
  flecs::entity m_siteEntity;

  std::vector<flecs::entity> m_manuBuildings;
  std::vector<flecs::entity> m_storageBuildings;
};
