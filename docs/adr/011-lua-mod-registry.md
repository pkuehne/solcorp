# 011 — Lua Mod Registry and Prefab Override System

**Status:** Accepted

## Context

SolCorp uses a Paradox/Factorio-inspired modding architecture: a core mod defines baseline game
entities (buildings, prefabs, etc.) as Lua tables, and subsequent mods can extend, override, or
suppress those definitions. The final merged registry is validated and mapped to Flecs ECS prefabs at
load time.

Two design questions needed explicit answers:

1. **How does a mod declare an entity?** The call should be equally valid for initial definition,
   patching, and no-ops — mods should not need to distinguish between "I am the first to touch this"
   and "I am amending someone else's entry."
2. **How does a mod suppress an entity entirely?** A full overhaul (fire-planet scenario,
   alternate-timeline mod) may need to prevent a core entity from ever becoming a prefab, while still
   leaving the registry entry intact so that later mods can patch or re-enable it.

This sits on the project's existing Lua layer, and the design below only makes sense against it:

- **Binding.** The C++/Lua boundary is a hand-rolled raw Lua C API (`lua_register_function`,
  `register_component_lua`, `register_enum_table_lua` in
  [lua.h](../../src/modules/lua/lua.h)). Sol3 is vendored under `dist/` but unused — evaluated and
  dropped. C++ owns the typed vocabulary; Lua supplies values.
- **Today's declaration flow is imperative.** A building is declared by
  `solcorp.helpers.create_building_prefab(name)`, which returns a real Flecs prefab entity under
  `Prefabs::Buildings` built `is_a(Prefabs::Core::Building)`
  ([helpers.cpp](../../src/modules/lua/helpers.cpp)), followed by mutating calls such as
  `building:setSprite(sprite)`. There is no table-merge registry, no override/suppress concept, and
  only the `core` mod exists today.

This ADR adds a Lua-table **staging registry** in front of that imperative flow. The merged table for
each id is mapped, at load time, onto the same Flecs prefab machinery that exists now
(`world.prefab(name).is_a(Prefabs::Core::Building).child_of(Prefabs::Buildings)`); the flags (§3) and
animations (§4) resolve to components/tags on that prefab, exactly as `setSprite` sets `Sprite` today.
Everything below concerns the Lua side up to that mapping point.

## Decision

### 1. Registry via deep merge

> Amendment (2026-06): the declaration surface is a **returned table**, not imperative `register`
> calls. Each mod *may* include a dedicated `buildings.lua` that **returns a table** of
> `id → definition`; the engine collects each mod's table and deep-merges them in load order. The
> `register(...)` signature below is retained as the *internal* merge primitive the engine calls per
> entry while folding a mod's returned table in — `mod_name`/`load_order` still drive provenance and
> merge order — but mods no longer call it directly. The merge semantics, `DELETE` sentinel,
> provenance, flags, and validation in the rest of this ADR are unchanged.

All mod definitions are applied through a single
`buildings.register(id, overrides, mod_name, load_order)` call. Internally this performs a deep merge
onto the existing registry entry, creating a new entry if none exists:

```lua
local function deep_merge(base, overrides)
    for k, v in pairs(overrides) do
        if type(v) == "table" and type(base[k]) == "table" then
            deep_merge(base[k], v)
        else
            base[k] = v
        end
    end
    return base
end
```

A `DELETE` sentinel removes individual fields without clearing the whole entry:

```lua
local DELETE = "__DELETE__"
```

The call is intentionally ambiguous — it covers initial definition, patching, and no-ops equally.
Mods do not need to know whether an entry exists before calling `register`.

### 2. Provenance tracking at prefab level

Each registry entry tracks which mods touched it and in what order, but only when a merge actually
produces a change:

```lua
function buildings.register(id, overrides, mod_name, load_order)
    local entry = registry[id] or { _data = {}, _history = {} }
    if deep_merge_changed(entry._data, overrides) then
        table.insert(entry._history, { source = mod_name, order = load_order })
    end
    registry[id] = entry
end
```

`deep_merge_changed` returns `true` if any value was actually written and differed from the existing
value. Provenance is per-prefab, not per-field — sufficient for the debug screen without per-field
overhead.

### 3. Distinct visibility and behaviour flags

`hidden` is reserved strictly for mod-overhaul suppression ("do not create a prefab for this entry at
all"). Separate flags handle other placement and access concerns:

| Flag | Meaning |
|---|---|
| `hidden = true` | No prefab created; entry exists in registry only |
| `spawnable = false` | Prefab exists; nothing should place it (template or internal use) |
| `buildable = false` | Engine and events can place it; the player build UI cannot |

This avoids double-booking flag semantics. A scenario prop such as a crashed Starship is
`buildable = false, spawnable = true, hidden = false`:

```lua
buildings.register("crashed_starship", {
    tileset = "wreckage",
    buildable = false,
    spawnable = true,
    hidden = false,
})
```

> **Naming note — `buildable` here vs. `Buildable` in ADR 009.** These are two different axes that
> compose, not a conflict. [ADR 009 §1](/adr/009-build-mode-and-site-window.md) defines `Buildable`
> as a *site-level relationship* (`Site --Buildable--> Prefabs::Buildings`) that decides which prefab
> *categories* a site accepts and therefore which palette sections the Site Window renders. The
> per-prefab `buildable` flag here decides whether an *individual* prefab within an accepted category
> is offered to the player. A site accepts the Buildings category; a `buildable = false` building in
> that category is still hidden from the player palette while remaining placeable by engine and events.

### 4. Animation metadata in prefab definitions

Animation definitions are first-class fields in the prefab table and benefit from deep merge like any
other data:

```lua
buildings.register("launch_gantry", {
    tileset = "gantry",
    animations = {
        idle    = { frames = {1, 2, 3},    fps = 4,  loop = true  },
        retract = { frames = {4, 5, 6, 7}, fps = 12, loop = false },
        extend  = { frames = {7, 6, 5, 4}, fps = 12, loop = false },
    }
})
```

A cosmetic mod can patch a single animation parameter without redefining the block:

```lua
buildings.register("launch_gantry", {
    animations = { idle = { fps = 8 } }
})
```

### 5. Post-merge validation before ECS mapping

After all mods have loaded and the registry is fully merged, a validation pass checks each entry for
required fields. Broken entries are logged — including which mod last touched them via `_history` —
and skipped; no prefab is created. No crash, no hard error. Mods that instantiate prefabs are expected
to check for existence first; a missing prefab due to a broken definition is a recoverable, logged
condition. (ADR 012 layers its own tile-registry validation on top of this same post-load pass.)

### 6. Dev override mod pattern

A `dev_overrides` mod excluded from the release load order provides a clean way to tweak costs,
capacities, or build times during development without touching core mod files. The debug screen's
provenance history will flag it as active if accidentally included in a build:

```lua
-- mods/dev_overrides/buildings.lua
buildings.register("habitat", {
    max_facilities = 999,
    cost = { credits = 0 },
})
```

### Load order

> Prerequisite, not current state: mods are loaded today by iterating the `mods/` directory in
> unspecified filesystem order, with no manifest and no dependency declaration
> ([lua.cpp](../../src/modules/lua/lua.cpp) `load_all_mods`). The `load_order` argument to `register`
> and the manifest below do not exist yet — this ADR introduces them. Deep merge is only deterministic
> once a defined load order exists, so that ordering is a hard prerequisite of this design.

Mods declare dependencies via manifest. Load order determines merge order; later mods win on field
conflicts. A mod that hides an entry that a later mod then patches is not an error — the patch is
applied to registry data; whether a prefab is ultimately created is decided by the `hidden` flag at
validation time:

```
core → scenario mod → overhaul mod → cosmetic mod → [dev_overrides]
```

## Consequences

- Registry is the single source of truth for all entity definitions up to ECS mapping time.
- Deep merge enables cosmetic and balance mods to be minimal — only declaring what they change.
- Full overhaul mods can suppress core content via `hidden` without breaking the registry for mods
  that do not interact with those entries.
- Provenance tracking makes a Factorio-style mod-history debug screen per prefab straightforward to
  implement.
- Flag semantics are unambiguous; each flag answers one question and has no overlap with the others.
- Validation is deferred to post-load; individual `register` calls are dumb and fast.
- A `dev_overrides` mod provides a clean development escape hatch with no risk of touching core mod
  files.

## Related

- [ADR 009](/adr/009-build-mode-and-site-window.md) — Build Mode and Site Window (consumes the
  `buildable` flag and the `Buildable` relationship; the player palette is the "player build UI"
  referenced by §3).
- [ADR 012](/adr/012-building-tileset-metadata.md) — Data-Driven Building Tileset Metadata (the tile
  registry and its validation build on this merge/override pattern).
