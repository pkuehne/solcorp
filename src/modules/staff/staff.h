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
  Stat motivation = Stat("motivation", "Motivation",
                         "How well motivated this employee is", 50.0);
  Stat leadership_skill =
      Stat("leadership_skill", "Leadership Skills",
           "How good this person is at leading others", 0.0);
  Stat domain_skill = Stat("domains_skills", "Domain Skills",
                           "How good this person is at their job", 0.0);
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
