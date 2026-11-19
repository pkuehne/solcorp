# Modding

SolCorp's game content is defined entirely in Lua. The core game content in `mods/core/` is itself a mod — there is no hard-coded content in C++.

## Contents

- [Getting Started](modding/getting-started.md) — Create your first mod
- [Lua API Reference](modding/lua-api-reference.md) — Full API surface

## Mod Structure

Mods live in the `mods/` directory. Each mod is a directory containing an `init.lua` entry point:

```
mods/
  core/
    init.lua        ← loaded automatically
    buildings.png   ← assets referenced by init.lua
  my-mod/
    init.lua
```

## Building Tiles

A draggable building is drawn as a 9-slice from tiles declared in the mod's
`tiles.lua`, keyed by building id (see
[ADR 012](adr/012-building-tileset-metadata.md)):

```lua
return {
  factory = {
    tileset = "factory", -- from tilesets.lua; every index refers to it
    roof_a = { index = 4, roles = { "Centre" }, weight = 3, surface = "Roof" },
    roof_b = { index = 5, roles = { "Centre" } },
    corner = { index = 0, roles = { "TopLeft" } },
    -- ...
  },
}
```

| Field | Meaning |
|---|---|
| `index` | Tile index into the tileset (required) |
| `roles` | Where the tile may go: `TopLeft`, `Top`, `TopRight`, `Left`, `Centre`, `Right`, `BottomLeft`, `Bottom`, `BottomRight` (required) |
| `weight` | Relative chance among tiles for the same role (default `1`) |
| `surface` | `Roof` or `SouthWall`; omit for none |
| `predicate` | A flag the building must have for the tile to show; omit for always |
| `tags` | List of gameplay tags |

Every other table in a building's entry is a tile, named by its key. Later mods
can override or `"__DELETE__"` a single tile by that name.

Invalid tiles are logged and skipped. Unless the building is `fixed_size`, every
role needs at least one tile without a `predicate`, or the building gets no
tiles at all.

## Connectors

Roads and other connectors are declared in a mod's `connectors.lua`, keyed by
id. Each becomes a prefab under `Prefabs::Connectors`:

```lua
return {
  road = { name = "Road", base = "roads_asphalt", markings = "roads_markings" },
}
```

`base` and `markings` name tilesets from `tilesets.lua`; `markings` is optional.
A connector whose `base` tileset is missing is logged and skipped.

## Lifecycle Handlers

Register handlers on the `solcorp.script.handlers` table:

```lua
solcorp.script.handlers.on_init   = function() end  -- once, before on_start
solcorp.script.handlers.on_start  = function() end  -- once, after all mods init
solcorp.script.handlers.on_update = function() end  -- every game update tick
solcorp.script.handlers.on_frame  = function() end  -- every rendered frame
```
