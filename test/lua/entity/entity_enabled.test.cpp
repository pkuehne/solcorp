#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:enabled() returns true for a new entity", "[lua][entity]") {
  GIVEN("a newly created entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("EnabledTest");
    set_entity_global(L, e);

    WHEN("e:enabled() is called") {
      int rc = luaL_dostring(L, "return e:enabled()");
      THEN("it returns true") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_toboolean(L, -1) == 1);
      }
    }
    lua_close(L);
  }
}
