#pragma once

#include "gui/launch_window.h"
#include "gui/main_menu.h"
#include "gui/site_window.h"
#include <string>

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

struct Construction {
  u_int effort_remaining = 0;
  u_int effort_total = 0;
};

struct CargoHold {
  u_int capacity = 0;
};

struct LaunchPlan {
  u_int launch_date = 0;
  bool draft = true;
};

struct Site {
  std::string name;
  u_int width = 10;
  u_int height = 10;
};

/// @brief Indicates that entity is a building
struct Building {
  std::string name = "";
};

struct SiteLocation {
  u_int x = 0;
  u_int y = 0;
};

/// @brief Allows construction of rockets
struct Manufacturing {
  u_int lines = 1;
  u_int max_weight = 1000;
  u_int available_effort = 10;
};

/// @brief Can launch rockets
struct Launchpad {
  u_int max_weight = 1000;
};

/// @brief For rockets and payloads
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

struct LaunchingFrom {}; /// From which launchpad?
struct LaunchingOn {};   /// On what  rocket
struct LaunchingWith {}; /// With what payloads?

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
  LaunchWindow launch_window;
  MainMenu main_menu;
};

struct OpenLaunchWindow {};
struct OpenSiteWindow {};
