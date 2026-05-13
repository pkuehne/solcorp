#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("lua_check_entity roundtrips a pushed entity", "[lua][entity]") {
  GIVEN("a lua state with an entity pushed on the stack") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity original = world.entity("RoundTrip");
    lua_push_entity(L, original);

    WHEN("lua_check_entity reads it back") {
      flecs::entity recovered = lua_check_entity(L, -1);
      THEN("the recovered entity has the same id") {
        REQUIRE(recovered.id() == original.id());
      }
    }
    lua_close(L);
  }
}

SCENARIO("lua_check_entity raises a Lua error for wrong type",
         "[lua][entity]") {
  GIVEN("a lua state with the entity metatable registered") {
    flecs::world world;
    lua_State *L = make_entity_state(world);

    WHEN("a non-entity value is passed to a function expecting an entity") {
      lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_check_entity(L, 1);
        return 0;
      });
      lua_pushstring(L, "not an entity");
      int rc = lua_pcall(L, 1, 0, 0);

      THEN("lua_pcall returns an error") { REQUIRE(rc != LUA_OK); }
    }
    lua_close(L);
  }
}
