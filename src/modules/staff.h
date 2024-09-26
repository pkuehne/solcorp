#pragma once

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

struct StaffModule {
  StaffModule(flecs::world &);
};
