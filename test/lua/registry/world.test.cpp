#include "modules/lua/lua_registry.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("lua_get_world returns nullptr when not set") {
  GIVEN("a fresh Lua state") {
    lua_State *L = luaL_newstate();
    THEN("lua_get_world returns nullptr") {
      REQUIRE(lua_get_world(L) == nullptr);
    }
    lua_close(L);
  }
}

SCENARIO("lua_set_world / lua_get_world roundtrip") {
  GIVEN("a raw Lua state and a flecs world") {
    lua_State *L = luaL_newstate();
    flecs::world world;
    WHEN("the world pointer is stored") {
      lua_set_world(L, &world);
      THEN("the same pointer is returned") {
        REQUIRE(lua_get_world(L) == &world);
      }
    }
    lua_close(L);
  }
}

SCENARIO("world pointer can be overwritten") {
  GIVEN("a Lua state with a world already stored") {
    lua_State *L = luaL_newstate();
    flecs::world world1;
    flecs::world world2;
    lua_set_world(L, &world1);
    WHEN("the world pointer is updated") {
      lua_set_world(L, &world2);
      THEN("the new pointer is returned") {
        REQUIRE(lua_get_world(L) == &world2);
      }
    }
    lua_close(L);
  }
}
