#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:child_of() sets parent relationship", "[lua][entity]") {
  GIVEN("a parent and child entity both pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity parent = world.entity("Parent");
    flecs::entity child = world.entity("Child");
    lua_push_entity(L, parent);
    lua_setglobal(L, "parent");
    lua_push_entity(L, child);
    lua_setglobal(L, "child");

    WHEN("child:child_of(parent) is called from Lua") {
      int rc = luaL_dostring(L, "child:child_of(parent)");
      THEN("the child has a ChildOf relationship with the parent") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(child.has(flecs::ChildOf, parent));
      }
    }
    lua_close(L);
  }
}
