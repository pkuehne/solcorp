#pragma once

#include "gui/site_window.h"
#include <flecs.h>
#include <string>
#include <sys/types.h>

void registerComponents(flecs::world &);

/// @brief Any human person
struct Person {
  std::string first_name;
  std::string last_name;
};

/// @brief An Employee of the company
struct Employee {
  int start = 0;
  u_int domain_skill = 50;
  u_int leadership_skill = 0;
  u_int motivation = 100;
};

/// @brief A team in the company
struct Team {
  std::string name;
};

struct Rocket {};

struct CargoHold {
  u_int capacity = 0;
};

struct PlanetaryLaunch {
  u_int remaining_days = 0;
  bool completed = false;
};

struct Site {
  std::string name;
  u_int width = 10;
  u_int height = 10;
};

/// @brief Tag that entity is a building
struct Building {};

struct SiteLocation {
  u_int x = 0;
  u_int y = 0;
};

struct Launchpad {
  u_int max_weight = 1000;
};

struct Storage {
  u_int max_storage = 1000;
};

struct Office {
  u_int max_desks = 100;
};

struct Position {
  u_int x = 0;
  u_int y = 0;
};

// Relationships
struct TeamMember {};
struct Manager {};

struct LaunchingWith {};

// Resources
struct PrefabResource {
  flecs::entity rocket;
};

struct QueryResource {
  flecs::rule<Person> team_members;
};

struct GameResource {
  flecs::entity sim_speed;
  u_int day = 0;
};

struct GuiResource {
  bool show_demo_window = false;
  SiteWindow site_window;
};
