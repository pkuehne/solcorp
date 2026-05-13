#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:destroy() removes the entity from the world",
         "[lua][entity]") {
  GIVEN("a live entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("DestroyTest");
    set_entity_global(L, e);

    WHEN("e:destroy() is called and then is_alive checked via C++") {
      REQUIRE(luaL_dostring(L, "e:destroy()") == LUA_OK);
      THEN("the entity is no longer alive") { REQUIRE(!e.is_alive()); }
    }
    lua_close(L);
  }
}
