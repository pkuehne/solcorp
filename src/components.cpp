#include "components.h"

void registerComponents(flecs::world &world) {
  world.component<TeamMember>().add(flecs::Transitive);
  world.component<Manager>().add(flecs::Symmetric);
}
