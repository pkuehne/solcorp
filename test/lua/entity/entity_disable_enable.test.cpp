#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:disable() and entity:enable() toggle enabled state",
         "[lua][entity]") {
  GIVEN("a live entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("ToggleTest");
    set_entity_global(L, e);

    WHEN("e:disable() is called then e:enable()") {
      REQUIRE(luaL_dostring(L, "e:disable()") == LUA_OK);
      REQUIRE(luaL_dostring(L, "e:enable()") == LUA_OK);
      int rc = luaL_dostring(L, "return e:enabled()");

      THEN("entity is enabled again") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_toboolean(L, -1) == 1);
      }
    }
    lua_close(L);
  }
}
