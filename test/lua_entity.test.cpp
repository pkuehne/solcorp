#include "modules/lua/entity.h"
#include "modules/lua/lua_registry.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

static lua_State *make_entity_state(flecs::world &world) {
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  lua_set_world(L, &world);
  load_entity_usertype(L);
  return L;
}

// Push e as a global "e" so Lua scripts can call methods on it.
static void set_entity_global(lua_State *L, flecs::entity e) {
  lua_push_entity(L, e);
  lua_setglobal(L, "e");
}

// ── lua_push_entity / lua_check_entity ───────────────────────────────────────

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

// ── entity methods via Lua ────────────────────────────────────────────────────

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

SCENARIO("entity:is_alive() reflects entity liveness", "[lua][entity]") {
  GIVEN("a live entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("AliveTest");
    set_entity_global(L, e);

    WHEN("e:is_alive() is called while entity is alive") {
      int rc = luaL_dostring(L, "return e:is_alive()");
      THEN("it returns true") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_toboolean(L, -1) == 1);
      }
    }
    lua_close(L);
  }
}

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

SCENARIO("entity:enabled() returns true for a new entity", "[lua][entity]") {
  GIVEN("a newly created entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("EnabledTest");
    set_entity_global(L, e);

    WHEN("e:enabled() is called") {
      int rc = luaL_dostring(L, "return e:enabled()");
      THEN("it returns true") {
        REQUIRE(rc == LUA_OK);
        REQUIRE(lua_toboolean(L, -1) == 1);
      }
    }
    lua_close(L);
  }
}

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

SCENARIO("entity:destroy() removes the entity from the world",
         "[lua][entity]") {
  GIVEN("a live entity pushed to Lua") {
    flecs::world world;
    lua_State *L = make_entity_state(world);
    flecs::entity e = world.entity("DestroyTest");
    set_entity_global(L, e);

    WHEN("e:destroy() is called and then is_alive checked via C++") {
      REQUIRE(luaL_dostring(L, "e:destroy()") == LUA_OK);
      THEN("the entity is no longer alive") { REQUIRE(!e.is_alive()); }
    }
    lua_close(L);
  }
}

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
