#pragma once

#include <flecs.h>
#include <string>

struct Rocket;
struct Launchpad;
struct Building;

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

  flecs::query<Rocket> m_rocketQuery;
  flecs::entity m_rocket;
  std::string m_rocketDisplay;

  flecs::query<Launchpad, Building> m_launchpadQuery;
  flecs::entity m_launchpad;
  std::string m_launchpadDisplay;
};
