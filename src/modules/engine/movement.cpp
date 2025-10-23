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
  // Register components
  world.component<Velocity>()
      .member("x", &Velocity::x)
      .member("y", &Velocity::y);
  world.component<Expire>().member("millis", &Expire::millis);

  register_lua_user_type<Velocity>(world, "Velocity",
                                   [](sol::usertype<Velocity> &userType) {
                                     userType["x"] = &Velocity::x;
                                     userType["y"] = &Velocity::y;
                                   });
  register_lua_user_type<Expire>(world, "Expire",
                                 [](sol::usertype<Expire> &userType) {
                                   userType["millis"] = &Expire::millis;
                                 });

  // Register Systems
  world.system<const Velocity, Transform>("Apply Velocity")
      .kind(UpdatePhase)
      .each(systemApplyVelocity);
  world.system<Expire>("Expire entities")
      .kind(UpdatePhase)
      .each(systemUpdateExpiry);
}
