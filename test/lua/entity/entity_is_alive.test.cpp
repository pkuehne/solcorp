#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:is_alive() reflects entity liveness", "[lua][entity]") {
  GIVEN("a live entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("AliveTest");
    set_entity_global(L, e);

    WHEN("e:is_alive() is called while entity is alive") {
      int rc = luaL_dostring(L, "return e:is_alive()");
      THEN("it returns true") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_toboolean(L, -1) == 1);
      }
    }
    lua_close(L);
  }
}
