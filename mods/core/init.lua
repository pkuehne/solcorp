local function on_init()
	-- init function
	print("on_init called!")
	local info = solcorp.logging.info
	info("on_init called!")
	local e = solcorp.entities.create("mod_entity")
	info("Entity: " .. e:id())
	e:disable()
	info("Entity is enabled " .. tostring(e:enabled()))
	e:enable()
	info("Entity is enabled " .. tostring(e:enabled()))
	e:destroy()
end

local function on_start()
	local info = solcorp.logging.info
	info("on_start called!")

	local comp = solcorp.entities.GameComponent()
	comp.day = 12
	info("Day: " .. comp.day)

	-- Create Texture
	solcorp.helpers.create_texture("Buildings", "buildings.png")

	-- Create Prefabs
	local sprite
	local lp_prefab = solcorp.helpers.create_building_prefab("Launchpad")
	lp_prefab:getLaunchpad()
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 96, 32, 32)
	lp_prefab:setSprite(sprite)

	local office_prefab = solcorp.helpers.create_building_prefab("Office Building")
	office_prefab:getOffice()
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 32, 32, 32)
	office_prefab:setSprite(sprite)

	local storage_prefab = solcorp.helpers.create_building_prefab("Storage Hall")
	storage_prefab:getStorage()
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 64, 32, 32)
	storage_prefab:setSprite(sprite)

	local factory_prefab = solcorp.helpers.create_building_prefab("Factory")
	local manu = solcorp.components.Manufacturing.new(2)
	factory_prefab:setManufacturing(manu)
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 64, 32, 32)
	factory_prefab:setSprite(sprite)

	-- Create a new site
	local site = solcorp.helpers.create_site("Cape Canaveral", 10, 10, true)

	solcorp.helpers.create_building("Manufacturing A", "Factory", 1, 1, site)
	solcorp.helpers.create_building("Storage Hall 1", "Storage Hall", 0, 0, site)
	solcorp.helpers.create_building("Main Launchpad", "Launchpad", 1, 0, site)
	solcorp.helpers.create_building("North Building", "Office Building", 2, 0, site)
	local south_pad = solcorp.helpers.create_building("South Launchpad", "Launchpad", 5, 5, site)
	south_pad:getLaunchpad().max_weight.base = 5000

	local concrete = solcorp.helpers.create_effect("Better Concrete", site)
	local mod = solcorp.components.Modifier:new()
	mod.target_stat = "max-weight"
	mod.multiplicative = 1.2
	solcorp.helpers.add_modifier(concrete, mod)

	local cracks = solcorp.helpers.create_effect("Cracks detected", south_pad)
	mod.multiplicative = 0.4
	solcorp.helpers.add_modifier(cracks, mod)

	local struts = solcorp.helpers.create_effect("Reinforcing Struts", site)
	mod.multiplicative = 1.0
	mod.additive = 500
	solcorp.helpers.add_modifier(struts, mod)
end

solcorp.script.handlers.on_init = on_init
solcorp.script.handlers.on_start = on_start
solcorp.script.handlers.on_frame = nil
solcorp.script.handlers.on_update = nil
