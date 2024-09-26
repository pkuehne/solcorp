#pragma once

#include <flecs.h>
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

// Find all Persons that are TeamMember of a $team, where the $team is a
// TeamMember of anything
// i.e. Match any person with a team that has another team above it.
// team_members.each([](flecs::iter &it, size_t, Person &p)
//                   { std::cout << p.first_name << " " << p.last_name << " is
//                   a member of "
//                               << it.get_var("team").name() << std::endl;
//                               });
//   world.set<QueryResource>({
//       world.rule_builder<Person>()
//           .with<Employee>()
//           .with<TeamMember>()
//           .second("$team")
//           .with<TeamMember>(flecs::Any)
//           .src("$team")
//           .build(), // team_members
//   });
// Resources

// world.component<TeamMember>().add(flecs::Transitive);
// world.component<Manager>().add(flecs::Symmetric);

struct GameResource {
  flecs::entity sim_speed;
  u_int day = 0;
};
