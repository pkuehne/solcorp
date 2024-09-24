#pragma once

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

// Relationships
struct TeamMember {};
struct Manager {};

// Resources

struct GameResource {
  flecs::entity sim_speed;
  u_int day = 0;
};

struct GuiResource {
  bool show_demo_window = false;
  SiteWindow site_window;
};

struct OpenSiteWindow {};
