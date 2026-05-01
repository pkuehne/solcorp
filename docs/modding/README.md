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

## Lifecycle Handlers

Register handlers on the `solcorp.script.handlers` table:

```lua
solcorp.script.handlers.on_init   = function() end  -- once, before on_start
solcorp.script.handlers.on_start  = function() end  -- once, after all mods init
solcorp.script.handlers.on_update = function() end  -- every game update tick
solcorp.script.handlers.on_frame  = function() end  -- every rendered frame
```
