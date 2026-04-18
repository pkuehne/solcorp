# Getting Started with Modding

## Create a Mod

1. Create a directory under `mods/`:
   ```
   mods/my-mod/
   ```

2. Create `mods/my-mod/init.lua`:
   ```lua
   solcorp.script.handlers.on_start = function()
       solcorp.logging.info("My mod loaded!")

       -- Create a new launch site
       local site = solcorp.helpers.create_site("My Site", 8, 8, true)

       -- Create a building prefab
       local hq = solcorp.helpers.create_building_prefab("Headquarters")

       -- Place it at the site
       solcorp.helpers.create_building("HQ", "Headquarters", 0, 0, site)
   end
   ```

3. Run the game — your mod's `on_start` will be called after core content loads.

## Defining Rockets

```lua
local rocket = solcorp.helpers.create_rocket_prefab("My Rocket")
solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Low Orbit", 5000)
```

The second argument to `add_target_orbit_to_rocket` is the payload capacity in kg.

## Working with Textures

Place texture files in your mod directory, then load them:

```lua
solcorp.helpers.create_texture("MyTextures", "my_sprite_sheet.png")

local sprite = solcorp.helpers.clip_sprite_from_texture("MyTextures", x, y, w, h)
building:setSprite(sprite)
```

## Logging

```lua
solcorp.logging.debug("verbose detail")
solcorp.logging.info("general info")
solcorp.logging.error("something went wrong")
```

Logs appear in `solcorp.log` in the working directory.
