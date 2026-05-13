#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:name() returns the entity name", "[lua][entity]") {
  GIVEN("a named entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("NamedEntity");
    set_entity_global(L, e);

    WHEN("e:name() is called from Lua") {
      int rc = luaL_dostring(L, "return e:name()");
      THEN("it returns the entity name as a string") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "NamedEntity");
      }
    }
    lua_close(L);
  }
}
