# Lua API Reference

## `solcorp.logging`

```lua
solcorp.logging.debug(msg)   -- debug level
solcorp.logging.info(msg)    -- info level
solcorp.logging.error(msg)   -- error level
```

---

## `solcorp.entities`

### `create([name])`

Creates and returns a new entity. Optionally names it.

```lua
local e = solcorp.entities.create("my_entity")
local e = solcorp.entities.create()
```

### Entity methods

| Method | Description |
|--------|-------------|
| `e:id()` | Returns entity ID as string |
| `e:enable()` | Enable entity |
| `e:disable()` | Disable entity |
| `e:enabled()` | Returns bool |
| `e:destroy()` | Destroy entity |
| `e:child_of(parent)` | Set parent entity |

### Component accessors

For each component `Foo` exposed to Lua:

| Method | Description |
|--------|-------------|
| `e:getFoo()` | Get (or add) component, returns ref |
| `e:setFoo(v)` | Set component |
| `e:hasFoo()` | Returns bool |
| `e:removeFoo()` | Remove component |

**Available components:** `Transform`, `Velocity`, `Sprite`, `Text`, `Expire`, `Launchpad`, `Office`, `Storage`, `Manufacturing` *(and others — see `src/modules/lua/lua.cpp`)*

---

## `solcorp.helpers`

### Sites & Buildings

```lua
-- Create a launch site
local site = solcorp.helpers.create_site(name, width, height, active)

-- Create a building prefab definition
local prefab = solcorp.helpers.create_building_prefab(name)

-- Place a building instance at a site
local building = solcorp.helpers.create_building(name, prefab_name, x, y, site)

-- Add a named facility to a building prefab
local facility = solcorp.helpers.add_facility_to_building(building_prefab, facility_name)
-- Then set facility type: facility:getLaunchpad() / facility:getOffice() / etc.
```

### Rockets

```lua
-- Define a rocket type
local rocket = solcorp.helpers.create_rocket_prefab(name)

-- Add a supported orbit with payload capacity (kg)
solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Low Orbit", capacity_kg)
```

### Textures & Sprites

```lua
-- Load a texture (path relative to mod directory)
solcorp.helpers.create_texture(texture_name, filename)

-- Clip a sprite from a texture atlas (pixel coords)
local sprite = solcorp.helpers.clip_sprite_from_texture(texture_name, x, y, w, h)
```

### Contracts

```lua
-- Create a contract
local contract = solcorp.helpers.create_contract(
    name,          -- mission name
    client,        -- company name
    description,
    min_reward,
    max_reward
)

-- Add a payload to the contract
solcorp.helpers.create_contract_payload(contract, payload_name, mass_kg, target_orbit)

-- Get all existing contracts
local contracts = solcorp.helpers.get_all_contracts()
```

### Effects & Modifiers

```lua
-- Create an effect attached to a site or building
local effect = solcorp.helpers.create_effect(name, target_entity)

-- Create and apply a modifier
local mod = solcorp.components.Modifier:new()
mod.target_stat    = "max-weight"   -- stat name
mod.multiplicative = 1.2            -- multiplier (1.0 = no change)
mod.additive       = 0              -- flat addition
solcorp.helpers.add_modifier(effect, mod)
```

**Known stat names:** `max-weight`, `prep-days`

---

## `solcorp.components`

### `Modifier`

```lua
local mod = solcorp.components.Modifier:new()
mod.target_stat    = "stat-name"
mod.multiplicative = 1.0
mod.additive       = 0
```

---

## `solcorp.script.handlers`

```lua
solcorp.script.handlers.on_init   = function() end
solcorp.script.handlers.on_start  = function() end
solcorp.script.handlers.on_update = function() end
solcorp.script.handlers.on_frame  = function() end
```

Each handler is called once (init/start) or every tick/frame (update/frame). Only one handler per event is active — assigning a new function replaces the previous one.
