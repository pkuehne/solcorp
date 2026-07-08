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

> Resolved (2026-06): the manifest, dependency declaration, and `load_order` described here are now
> implemented — see the **Mod manifest and dependency resolution** section below. The text in this
> section describes the resulting behaviour.

Mods declare dependencies via manifest. Load order determines merge order; later mods win on field
conflicts. A mod that hides an entry that a later mod then patches is not an error — the patch is
applied to registry data; whether a prefab is ultimately created is decided by the `hidden` flag at
validation time:

```
core → scenario mod → overhaul mod → cosmetic mod → [dev_overrides]
```

## Mod manifest and dependency resolution

> Amendment (2026-06): this section promotes the "Load order" prerequisite above into a concrete,
> implemented decision. It introduces the manifest format, the dependency-resolution algorithm, and
> the failure policy. The merge/override semantics in §1–§6 are unchanged.

### Manifest format — `mod.lua`

A directory under `mods/` is recognised as a mod **iff it contains a `mod.lua` manifest**. `mod.lua`
is a Lua file that **returns a table** (read with no side effects, before `init.lua` runs). The
directory name is the canonical **id** that dependencies reference; `name` is a display string.
`init.lua` is now **optional** — a graphics/data-only mod with no event handlers omits it.

```lua
-- mods/core/mod.lua
return {
  name = "Core",              -- display name (optional; defaults to the directory id)
  version = "0.1.0",          -- "major.minor.patch"; missing components default to 0
  description = "...",        -- human-readable summary (optional)
  author = "SolCorp",         -- author / attribution (optional)
  dependencies = {
    -- "weather",                         -- any version
    -- { id = "weather", version = "0.2.0" },  -- weather >= 0.2.0
  },
}
```

The display `name`, `description`, and `author` are carried onto the `Mod`
component (alongside `version` and `load_order`) for the future
mod-history/about debug screen; only `id` (the directory name) and
`dependencies` participate in resolution.

Dependencies declare a **minimum version** only (a compatibility floor), not a full constraint
grammar — the actual need today is deterministic load ordering, and a floor is sufficient for that.

### Resolution

The engine reads every `mod.lua` up front into a manifest list, then resolves a deterministic load
order via **topological sort** (Kahn's algorithm; ties broken by id so the order is stable):
dependencies load before their dependents. The resolved index is stored as `Mod::load_order`
alongside `Mod::version`, and `run_on_every_mod` iterates mods in that order so every per-mod pass
(component/enum registration, the eventual registry merge) runs dependencies-first.

The manifest reader is built on a reusable `LuaDataFile` primitive ("open a Lua file that returns a
table, read typed fields") so the same path will serve building metadata (§1's `buildings.lua`),
ADR 012 tileset metadata, and configs.

### Failure policy — hard fail

Unlike the post-merge *content* validation in §5 (which logs and skips individual broken entries),
problems with the **mod graph itself** are unrecoverable: the game would otherwise start in an
undefined content state. A missing dependency, a dependency whose version is below the required
minimum, a dependency cycle, or a duplicate id is logged at `critical` (the cycle message names the
cycle, e.g. `a -> b -> a`) and the process exits non-zero. There is no partial-load fallback for a
broken dependency graph.

## Implementation (2026-07 amendment)

The merge/override system (§1–§6) is now implemented. It follows the design above with one
substantive refinement: **the deep merge runs in C++ over a materialised value tree, not in Lua.**

- **Why not Lua.** Each mod's data file is read in its own throwaway Lua state (`LuaDataFile`), so
  tables from different mods cannot be deep-merged while they live in separate states. Rather than
  introduce a long-lived shared Lua state plus embedded merge code, each returned table is
  materialised into a detached C++ value tree (`ModValue`, via `LuaDataFile::materialize()`) that
  outlives its Lua state. The whole merge is then pure C++ and unit-testable in the existing Catch2
  suite. The `register`/`deep_merge`/`_history` semantics of §1–§2 are preserved; only the host
  language changed.
- **`ModRegistry`** folds each mod's tree for a category (`buildings`, `rockets`, `effects`,
  `textures`) in resolved load order (`for_each_mod`). It keeps the `_data`/`_history` model of §2:
  provenance is appended only when a merge writes a differing value (`deep_merge_changed`).
- **Merge semantics.** Maps deep-merge (patch `sprite.x` or `animations.idle.fps` without touching
  siblings). **Lists replace wholesale** (`facilities`, animation `frames`) — a deliberate
  simplification of §1's pairs-based index merge, which is clearer for list-shaped fields. The
  `DELETE` sentinel (`"__DELETE__"`) removes a field, or removes a whole entry when it is the entire
  override value.
- **Post-merge validation (§5)** is a per-category pass over the merged table (`selectBuildingPrefabs`
  / `selectEffects` / `selectTextures`, and the orbit check in `applyRocketData`). Broken entries are
  logged with their provenance (`ModRegistry::historyString`) and skipped, never fatal — a rocket
  with an unknown orbit drops that lift capability, an effect with no modifiers is skipped, a texture
  with no file is skipped. `hidden` entries create no prefab. A texture's `file` is resolved against
  the mod that last supplied it (`ModRegistry::lastSource`), since a merged entry no longer knows its
  origin.
- **Flags (§3) and animations (§4)** deep-merge and are carried in the merged tree, but only `hidden`
  is mapped to the ECS so far. `spawnable`, `buildable`, and `animations` have no consumer yet (the
  player build UI is [ADR 009](/adr/009-build-mode-and-site-window.md); animation rendering and the
  tile registry are [ADR 012](/adr/012-building-tileset-metadata.md)), so they are intentionally left
  unapplied rather than mapped to unused components.
- **Dev overrides (§6).** A `dev_only = true` manifest field marks a mod (the existing `mods/dev`
  mod carries the flag and a `buildings.lua` override example) that is filtered out of the load order
  in release builds (`NDEBUG`) and loads normally in debug builds. The developer window's Mods tab
  warns when a `dev_only` mod is active.

## Consequences

- Registry is the single source of truth for all entity definitions up to ECS mapping time.
- Deep merge enables cosmetic and balance mods to be minimal — only declaring what they change.
- Full overhaul mods can suppress core content via `hidden` without breaking the registry for mods
  that do not interact with those entries.
- Provenance tracking makes a Factorio-style mod-history debug screen per prefab straightforward to
  implement.
- Flag semantics are unambiguous; each flag answers one question and has no overlap with the others.
- Validation is deferred to post-load; individual `register` calls are dumb and fast.
- A `dev_only` mod provides a clean development escape hatch with no risk of touching core mod
  files or shipping in a release build.

## Related

- [ADR 009](/adr/009-build-mode-and-site-window.md) — Build Mode and Site Window (consumes the
  `buildable` flag and the `Buildable` relationship; the player palette is the "player build UI"
  referenced by §3).
- [ADR 012](/adr/012-building-tileset-metadata.md) — Data-Driven Building Tileset Metadata (the tile
  registry and its validation build on this merge/override pattern).
