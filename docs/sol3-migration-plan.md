# Sol3 → Raw Lua C API Migration Plan

Sol3 (sol2) has been abandoned by its author. The Lua usage in this codebase is
simple enough to replace with the standard Lua C API directly. This document
records the staged migration plan so that each stage can be validated
independently before the next begins.

## Deferred decisions

- **config.lua loading** (`load_config_file` in `lua.cpp`): disabled for now,
  will be addressed in a separate pass. The function already does nothing
  critical at startup.
- **Config format**: Lua-scripted config feels right (avoids adding a toml/ini
  parser dependency). Decision deferred until the migration is complete.

## Current sol3 surface area

| Category | Files | Complexity |
|---|---|---|
| State ownership (`sol::state`) | `lua.h`, `lua.cpp` | Low |
| Script loading + protected calls | `lua.cpp` | Low |
| Table namespace setup | `lua.cpp`, `logging.cpp`, `helpers.cpp`, `entity.cpp` | Medium |
| `sol::this_state` + function registration | `logging.cpp`, `helpers.cpp`, `entity.cpp` | Medium |
| Usertype registration (entity + components) | `entity.cpp`, `lua.h` template | High |

All files are under `src/modules/lua/`.

---

## ~~Stage 1 — Extract pure business logic from Lua wrappers~~

**Goal:** Testability foundation. Every helper in `helpers.cpp` currently has two
jobs: extract the world pointer from the Lua state, then do ECS work. Split them.

Extract pure C++ overloads taking `flecs::world&` directly, e.g.:
```cpp
flecs::entity create_site(flecs::world& world, const std::string& name,
                          uint32_t width, uint32_t height, bool make_current);
```

The sol3 functions become thin shims that extract the world and delegate. Declare
the pure overloads in `helpers.h` and `entity.h`.

Functions to extract: `create_site`, `create_building_prefab`, `create_rocket_prefab`,
`add_facility_to_building`, `create_rocket`, `create_building`,
`add_target_orbit_to_rocket`, `create_texture`, `create_effect`, `add_modifier`,
`clip_sprite_from_texture`, `create_contract`, `create_contract_payload`,
`get_all_contracts`, `get_all_active_contracts`.

**Validation:**
- New unit tests for the extracted functions with no Lua involved.
  `get_all_contracts` / `get_all_active_contracts` have filter logic that
  should be covered.
- All existing tests still pass.

---

## ~~Stage 2 — Move world pointer and mod name to the Lua registry~~

**Goal:** Replace `mod_state["solcorp"]["world"].get<flecs::world*>()` with a Lua
registry lookup. The registry is private per-state and maps directly to raw API
calls without going through sol3 table chains.

Introduce a new header `src/modules/lua/lua_registry.h` with four helpers:

```cpp
void lua_set_world(lua_State* L, flecs::world* world);
flecs::world* lua_get_world(lua_State* L);
void lua_set_mod_name(lua_State* L, const std::string& name);
std::string lua_get_mod_name(lua_State* L);
```

Implemented with `lua_pushlightuserdata` / `lua_pushstring` and
`lua_setfield(L, LUA_REGISTRYINDEX, key)`. The sol3 side still reads
`["solcorp"]["world"]` until Stage 4 — both can coexist during the transition.

**Validation:**
- Unit test the four registry helpers against a raw `luaL_newstate()` (no game
  code needed).
- No game-level behaviour change.

---

## ~~Stage 3 — Replace logging (smallest end-to-end replacement)~~

**Goal:** Fully remove sol3 from `logging.cpp`.

Change `log_info(sol::this_state s, const std::string& message)` etc. to
`lua_CFunction`:

```cpp
static int log_info(lua_State* L) {
  const char* msg = luaL_checkstring(L, 1);
  std::string mod_name = lua_get_mod_name(L);   // Stage 2 helper
  auto logger = spdlog::get(mod_name);
  if (logger) logger->info("{}", msg);
  return 0;
}
```

Registration in `load_logging` switches from `logging_ns.set_function(...)` to
`lua_pushcfunction` + `lua_setfield`. The `load_logging` signature takes
`lua_State*` instead of `sol::state&`. Call sites in `lua.cpp` pass
`mod.L` (or `mod.state.lua_state()` if `Mod` still owns `sol::state` at this point).

**Validation:**
- Unit test: create a raw `lua_State*`, call `load_logging`, call the Lua
  functions via `lua_pcall`, assert log output (can use a custom spdlog sink).
- Existing tests still pass.
- Game boots with logging working in the core mod.

---

## ~~Stage 4 — Replace `Mod::state` with owned `lua_State*`~~

**Goal:** Remove `sol::state` from the `Mod` struct so it no longer transitively
pulls in `sol/sol.hpp` through `lua.h`.

```cpp
struct Mod {
  std::string name;
  lua_State* L;   // owned; closed via lua_close in destructor or unique_ptr deleter
};
```

Add a destructor (or custom deleter in a `unique_ptr<lua_State, LuaStateDeleter>`)
that calls `lua_close(L)`.

Sites that need to use sol3 APIs still present during this migration can
construct a temporary `sol::state_view sv(mod.L)` — sol3 allows non-owning
views from a raw pointer.

The `ModStateCallback` typedef and `run_on_every_mod` signature change to:
```cpp
typedef const std::function<void(lua_State*)> ModStateCallback;
void run_on_every_mod(flecs::world& world, const ModStateCallback& func);
```

Call sites construct `sol::state_view` from the passed `lua_State*` where still
needed.

**Validation:**
- Game boots, all mod handlers (`on_init`, `on_start`, `on_update`, `on_frame`)
  run correctly.
- The `Mod` component header no longer includes `sol/sol.hpp`.

---

## Stage 5 — Replace table namespace setup and script loading

**Goal:** Remove all `get_or_create<sol::table>()` chains and `safe_script_file` /
`sol::protected_function` from `lua.cpp`.

Replacements:

| sol3 | Raw Lua C API |
|---|---|
| `state["k"].get_or_create<sol::table>()` | `lua_getglobal` + `lua_newtable` + `lua_setglobal` via a helper `lua_ensure_table(L, parent, key)` |
| `mod_state.safe_script_file(path, sol::script_pass_on_error)` | `luaL_loadfile(L, path)` + `lua_pcall(L, 0, LUA_MULTRET, 0)` |
| `sol::protected_function f = ...; auto r = f()` | `lua_getfield` chain + `lua_pcall(L, 0, 0, 0)` |
| `sol::optional<sol::table> result; result.has_value()` | check `lua_pcall` return code |
| `sol::error err = result; err.what()` | `lua_tostring(L, -1)` after failed `lua_pcall` |

Write a utility `lua_ensure_table(lua_State* L, int parent_idx, const char* key)`
that pushes the named sub-table (creating it if absent), mirroring the
`get_or_create` pattern.

**Note on mod_name read-only property:** The previous sol3 `sol::property` that
made `solcorp.mod_name` read-only is replaced by a `__newindex` metamethod on the
`solcorp` table that raises a Lua error on any assignment to `mod_name`. Mods must
not be trusted to overwrite their own name.

```cpp
// Pseudocode for the read-only guard
lua_newtable(L);  // metatable for solcorp
lua_pushcfunction(L, [](lua_State* L) -> int {
    const char* key = lua_tostring(L, 2);
    if (strcmp(key, "mod_name") == 0)
        return luaL_error(L, "mod_name is read-only");
    lua_rawset(L, 1);
    return 0;
});
lua_setfield(L, -2, "__newindex");
lua_setmetatable(L, solcorp_table_idx);
```

**Validation:**
- Game boots, core mod loads, `on_init` / `on_start` / `on_update` all fire.
- Attempting to assign `solcorp.mod_name = "x"` from Lua raises an error.
- `load_config_file` is stubbed out / disabled (deferred decision).

---

## Stage 6 — Replace entity namespace functions and helper function registration

**Goal:** Remove `sol::this_state` from all functions in `helpers.cpp` and the
entity namespace functions in `entity.cpp`.

All helper functions become `lua_CFunction` with signature `static int fn(lua_State* L)`:

1. Read arguments off the stack with `luaL_checkstring`, `luaL_checkinteger`,
   `lua_check_entity` (Stage 7 helper, can be forward-declared).
2. Get world via `lua_get_world(L)` (Stage 2 helper).
3. Call the pure business logic extracted in Stage 1.
4. Push results back (entity userdata via `lua_push_entity`, table via
   `lua_newtable` + `lua_rawseti`).

`get_all_contracts` / `get_all_active_contracts` build a Lua table:
```cpp
lua_newtable(L);
int i = 1;
world->query_builder<Contract>().build().each([&](flecs::entity e, Contract&) {
    lua_push_entity(L, e);
    lua_rawseti(L, -2, i++);
});
return 1;
```

Registration uses `lua_pushcfunction` + `lua_setfield` throughout.

**Validation:**
- All Stage 1 unit tests still pass.
- Game starts, core mod `on_init` completes (creates textures, prefabs, sites,
  contracts).

---

## Stage 7 — Replace `flecs::entity` usertype

**Goal:** Remove `mod_state.new_usertype<flecs::entity>()` from `entity.cpp`.

`flecs::entity` is essentially a `uint64_t` — store it as fixed-size full userdata:

```cpp
flecs::entity* lua_push_entity(lua_State* L, flecs::entity e) {
    auto* ud = static_cast<flecs::entity*>(
        lua_newuserdata(L, sizeof(flecs::entity)));
    *ud = e;
    luaL_getmetatable(L, "solcorp.entity");
    lua_setmetatable(L, -2);
    return ud;
}

flecs::entity lua_check_entity(lua_State* L, int idx) {
    return *static_cast<flecs::entity*>(
        luaL_checkudata(L, idx, "solcorp.entity"));
}
```

Create the metatable once per Lua state via `luaL_newmetatable(L, "solcorp.entity")`.
Set `__index` to the method table. Register the 10 methods as `lua_CFunction`s:
`id`, `destroy`, `is_alive`, `name`, `symbol`, `enabled`, `enable`, `disable`,
`child_of`, `lookup`, `is_a`.

The `load_entity_usertype` function now takes `lua_State*` and does all of this
using raw Lua C API only.

**Validation:**
- Unit test: push an entity, call each method via `lua_pcall`, verify results.
- Unit test: `lua_check_entity` raises a Lua error when called with the wrong type.
- Integration: game starts, entity methods work in core mod Lua scripts.

---

## Stage 8 — Replace `register_lua_user_type<T>` component template

**Goal:** Remove the template in `lua.h` and all `sol::usertype<T>` usage for game
components.

Pattern per component type `T`:

```cpp
template <typename T>
void register_component_lua(
    lua_State* L,
    const char* name,
    const std::function<void(lua_State* L, int metatable_idx)>& register_fields
      = [](lua_State*, int) {})
{
    // 1. Create metatable "solcorp.T"
    std::string mt_name = std::string("solcorp.") + name;
    luaL_newmetatable(L, mt_name.c_str());
    int mt_idx = lua_gettop(L);
    register_fields(L, mt_idx);   // caller adds __index fields for component fields
    lua_pop(L, 1);

    // 2. Add getT / setT / hasT / removeT to "solcorp.entity" metatable
    luaL_getmetatable(L, "solcorp.entity");
    int entity_mt = lua_gettop(L);

    // getT: returns light userdata pointing at component memory, with metatable
    lua_pushstring(L, (std::string("get") + name).c_str());
    lua_pushcfunction(L, [](lua_State* L) -> int {
        auto e = lua_check_entity(L, 1);
        T* comp = &e.ensure<T>();
        lua_pushlightuserdata(L, comp);
        luaL_getmetatable(L, mt_name.c_str());
        lua_setmetatable(L, -2);
        return 1;
    });
    lua_rawset(L, entity_mt);

    // setT, hasT, removeT similarly...
    lua_pop(L, 1);  // pop entity metatable
}
```

The optional `register_fields` callback (previously `registerFunc` taking
`sol::usertype<T>&`) is now `std::function<void(lua_State*, int metatable_idx)>`,
which adds field accessors to the component metatable.

**Note:** Component field mutation via `__index` / `__newindex` on light userdata
needs care — the pointer is only valid as long as the ECS component is alive. This
is the same lifetime constraint that existed with sol3, just made more explicit.

**Validation:**
- Existing component tests (via C++ side) still pass.
- Integration: mod loading correctly creates and mutates components.

---

## Stage 9 — Remove sol3 from CMakeLists.txt

**Goal:** Clean removal. sol3 no longer appears anywhere in the codebase.

- Remove the FetchContent block for sol3 / sol2.
- Remove all remaining `#include <sol/sol.hpp>` and `#include <sol/types.hpp>`.
- Verify clean build with `-Wall -Wextra -Wpedantic -Werror`.

**Validation:**
- Full clean build from scratch (`cmake -B build -G Ninja && ninja`).
- All unit tests pass.
- Game boots and all mod content appears correctly.

---

## Stage ordering and parallelism

```
Stage 1  ─┐
Stage 2  ─┼─▶ Stage 3 ──▶ Stage 4 ──▶ Stage 5 ──▶ Stage 6 ──▶ Stage 7 ──▶ Stage 8 ──▶ Stage 9
           └─────────────────────────────────────────────────────▲
                                                (lua_push/check_entity needed here)
```

Stages 1 and 2 are independent of each other and can be done in either order or
in parallel. All subsequent stages depend on both 1 and 2 being complete.
Stage 7 (`flecs::entity` usertype) must be complete before Stage 8 (component
template) because Stage 8's getter/setter implementations call `lua_check_entity`.
