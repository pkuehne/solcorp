#include "components.h"

void registerComponents(flecs::world &world) {
  world.component<TeamMember>().add(flecs::Transitive);
  world.component<Manager>().add(flecs::Symmetric);
  world.component<LaunchingWith>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingFrom>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingOn>().add(flecs::Exclusive).add(flecs::Symmetric);
}
