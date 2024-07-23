#include "building_systems.h"
#include "components.h"

void systemBuildingUpdateConstruction(flecs::entity entity,
                                      Manufacturing &manufacturing) {
  flecs::world world = entity.world();

  entity.children([&](flecs::entity r) {
    Construction *construction = r.get_mut<Construction>();
    if (!construction)
      return;
    if (construction->effort_remaining == 0) {
      r.remove<Construction>();
      return;
    }
    if (manufacturing.available_effort > construction->effort_remaining) {
      construction->effort_remaining = 0;
    } else {
      construction->effort_remaining -= manufacturing.available_effort;
    }
  });
}
