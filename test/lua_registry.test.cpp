#include "modules/lua/lua_registry.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>
#include <stdexcept>

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

SCENARIO("lua_get_mod_name throws when not set") {
  GIVEN("a fresh Lua state with no mod name stored") {
    lua_State *L = luaL_newstate();
    THEN("lua_get_mod_name throws std::runtime_error") {
      REQUIRE_THROWS_AS(lua_get_mod_name(L), std::runtime_error);
    }
    lua_close(L);
  }
}

SCENARIO("lua_set_mod_name / lua_get_mod_name roundtrip") {
  GIVEN("a raw Lua state") {
    lua_State *L = luaL_newstate();
    WHEN("a mod name is stored") {
      lua_set_mod_name(L, "core");
      THEN("the same name is returned") {
        REQUIRE(lua_get_mod_name(L) == "core");
      }
    }
    lua_close(L);
  }
}

SCENARIO("mod name can be overwritten") {
  GIVEN("a Lua state with a mod name already set") {
    lua_State *L = luaL_newstate();
    lua_set_mod_name(L, "old_name");
    WHEN("a new mod name is stored") {
      lua_set_mod_name(L, "new_name");
      THEN("the new name is returned") {
        REQUIRE(lua_get_mod_name(L) == "new_name");
      }
    }
    lua_close(L);
  }
}

SCENARIO("world and mod name are independent registry entries") {
  GIVEN("a Lua state") {
    lua_State *L = luaL_newstate();
    flecs::world world;
    WHEN("both are set") {
      lua_set_world(L, &world);
      lua_set_mod_name(L, "core");
      THEN("each is retrieved correctly") {
        REQUIRE(lua_get_world(L) == &world);
        REQUIRE(lua_get_mod_name(L) == "core");
      }
    }
    lua_close(L);
  }
}
