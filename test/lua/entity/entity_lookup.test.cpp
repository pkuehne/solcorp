#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:lookup() finds a named child entity", "[lua][entity]") {
  GIVEN("a parent with a named child pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity parent = world.entity("LookupParent");
    world.entity("LookupChild").child_of(parent);
    set_entity_global(L, parent);

    WHEN("e:lookup('LookupChild') is called from Lua") {
      int rc = luaL_dostring(L, "return e:lookup('LookupChild')");
      THEN("it returns a valid entity userdata") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_isuserdata(L, -1));
        flecs::entity found = lua_check_entity(L, -1);
        REQUIRE(found.is_valid());
      }
    }
    lua_close(L);
  }
}
