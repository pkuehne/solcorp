#include "movement.h"
#include "modules/base/base.h"
#include "modules/engine/render.h"
#include "modules/lua/lua.h"

void systemApplyVelocity(flecs::iter &it, size_t, const Velocity &v,
                         Transform &t) {
  t.relativePosition.x += (v.x * it.delta_time());
  t.relativePosition.y += (v.y * it.delta_time());
}

void systemUpdateExpiry(flecs::iter &it, size_t row, Expire &e) {
  e.millis -= (it.delta_time() * 1000);
  if (e.millis <= 0) {
    it.entity(row).destruct();
  }
}

void registerMovement(flecs::world &world) {
  register_component_lua<Velocity>(world, "Velocity", [](lua_State *L, int mt) {
    lua_register_field<&Velocity::x>(L, mt, "x");
    lua_register_field<&Velocity::y>(L, mt, "y");
  });
  register_component_lua<Expire>(world, "Expire", [](lua_State *L, int mt) {
    lua_register_field<&Expire::millis>(L, mt, "millis");
  });

  // Register Systems
  world.system<const Velocity, Transform>("Apply Velocity")
      .kind(UpdatePhase)
      .each(systemApplyVelocity);
  world.system<Expire>("Expire entities")
      .kind(UpdatePhase)
      .each(systemUpdateExpiry);
}
