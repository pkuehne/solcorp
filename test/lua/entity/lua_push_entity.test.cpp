#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("lua_push_entity pushes a userdata with solcorp.entity metatable",
         "[lua][entity]") {
  GIVEN("a lua state with the entity metatable registered") {
    flecs::world world;
    lua_State *L = make_entity_state(world);

    WHEN("an entity is pushed") {
      flecs::entity e = world.entity("PushTest");
      lua_push_entity(L, e);

      THEN("the top of the stack is userdata with the correct metatable") {
        REQUIRE(lua_isuserdata(L, -1));
        luaL_getmetatable(L, "solcorp.entity");
        REQUIRE(lua_getmetatable(L, -2) != 0);
        REQUIRE(lua_rawequal(L, -1, -2));
      }
    }
    lua_close(L);
  }
}
