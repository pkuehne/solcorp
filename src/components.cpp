#include "components.h"
#include <flecs.h>

void registerComponents(flecs::world &world) {
  world.component<TeamMember>().add(flecs::Transitive);
  world.component<Manager>().add(flecs::Symmetric);
  world.component<LaunchingWith>().add(flecs::Exclusive).add(flecs::Symmetric);
}
