#include "test_helpers.h"
#include <catch2/catch_test_macros.hpp>

SCENARIO("entity:is_a() with a missing prefab logs an error and does nothing",
         "[lua][entity]") {
  GIVEN("an entity and a lua state with no Prefabs hierarchy") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("IsATest");
    set_entity_global(L, e);

    WHEN("e:is_a('NonExistentPrefab') is called") {
      int rc = luaL_dostring(L, "e:is_a('NonExistentPrefab')");
      THEN("it completes without Lua error") { REQUIRE(rc == LUA_OK); }
    }
    lua_close(L);
  }
}
