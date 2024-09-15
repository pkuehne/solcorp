#pragma once

#include <flecs.h>

class LaunchWindow {
public:
  void show(const flecs::entity &site);
  void hide();

  void draw(flecs::world &world);
  void loadData();

private:
  flecs::entity m_entity;
  bool m_visible = false;
  int m_launchDay = 0;
  u_int m_launchPrepDays = 5;
};
