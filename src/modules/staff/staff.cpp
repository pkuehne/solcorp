#include "staff.h"
#include <spdlog/spdlog.h>

void systemSeedStaff(flecs::iter &it);

StaffModule::StaffModule(flecs::world &world) {
  // Register components
  world.component<Person>()
      .member("first_name", &Person::first_name)
      .member("last_name", &Person::last_name);

  world.component<Employee>()
      .member("start", &Employee::start)
      .member("domain_skill", &Employee::domain_skill)
      .member("leadership_skill", &Employee::leadership_skill)
      .member("motivation", &Employee::motivation);

  world.component<Team>().member("name", &Team::name);

  // Register relationships
  world.component<TeamMember>().add(flecs::Transitive);
  world.component<Manager>().add(flecs::Symmetric);

  // Register systems
  world.system("Seed Staff").kind(flecs::OnStart).run(systemSeedStaff);
}

void systemSeedStaff(flecs::iter &it) {
  spdlog::debug("Creating teams and employees");
  const flecs::world &world = it.world();

  auto finance = world.entity("Finance").set<Team>({"Finance"});
  auto purchasing = world.entity("Purchasing")
                        .set<Team>({"Purchasing"})
                        .add<TeamMember>(finance);
  auto payable = world.entity("Payable")
                     .set<Team>({"Accounts Payable"})
                     .add<TeamMember>(finance);

  world.entity("Anna")
      .set<Person>({"Anna", "Delaney"})
      .add<Employee>()
      .add<TeamMember>(purchasing);
  world.entity("Bernardo")
      .set<Person>({"Bernardo", "Jimenez"})
      .add<Employee>()
      .add<TeamMember>(purchasing);
  world.entity("Kathy")
      .set<Person>({"Kathy", "Lau"})
      .add<Employee>()
      .add<TeamMember>(purchasing)
      .add<Manager>(purchasing);
  world.entity("Dave")
      .set<Person>({"Dave", "Jones"})
      .add<Employee>()
      .add<TeamMember>(finance)
      .add<Manager>(finance);
  world.entity("Elena")
      .set<Person>({"Elena", "Rodriguez"})
      .add<Employee>()
      .add<TeamMember>(payable)
      .add<Manager>(payable);
  world.entity("Freddie")
      .set<Person>({"Fred", "Dorn"})
      .add<Employee>()
      .add<TeamMember>(payable);
}

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
