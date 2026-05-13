#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:id() returns the entity id", "[lua][entity]") {
  GIVEN("a named entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("IdTest");
    set_entity_global(L, e);

    WHEN("e:id() is called from Lua") {
      int rc = luaL_dostring(L, "return e:id()");
      THEN("it returns a non-zero integer matching the entity id") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_isinteger(L, -1));
        REQUIRE(static_cast<uint64_t>(lua_tointeger(L, -1)) == e.id());
      }
    }
    lua_close(L);
  }
}
