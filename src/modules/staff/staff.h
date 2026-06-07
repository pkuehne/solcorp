#pragma once

#include "modules/stats/stats.h"
#include <flecs.h>
#include <string>

/// @brief Any human person
struct Person {
  std::string first_name;
  std::string last_name;
};

/// @brief An Employee of the company
struct Employee {
  int start = 0;
  Stat motivation = Stat({.id = "motivation", .base = 50.0});
  Stat leadership_skill = Stat({.id = "leadership_skill"});
  Stat domain_skill = Stat({.id = "domains_skills"});
};

/// @brief A team in the company
struct Team {
  std::string name;
};

// Relationships
struct TeamMember {};
struct Manager {};

struct StaffModule {
  StaffModule(flecs::world &);
};
