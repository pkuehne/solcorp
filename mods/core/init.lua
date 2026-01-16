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
	local building
	local facility

	building = solcorp.helpers.create_building_prefab("Launch Complex")
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 96, 32, 32)
	building:setSprite(sprite)
	facility = solcorp.helpers.add_facility_to_building(building, "Launchpad A")
	facility:getLaunchpad()

	building = solcorp.helpers.create_building_prefab("Office Building")
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 32, 32, 32)
	building:setSprite(sprite)
	facility = solcorp.helpers.add_facility_to_building(building, "Floor 1")
	facility:getOffice()

	building = solcorp.helpers.create_building_prefab("Storage Hall")
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 64, 32, 32)
	building:setSprite(sprite)
	facility = solcorp.helpers.add_facility_to_building(building, "Hall 1")
	facility:getStorage()

	building = solcorp.helpers.create_building_prefab("Factory")
	sprite = solcorp.helpers.clip_sprite_from_texture("Buildings", 0, 64, 32, 32)
	building:setSprite(sprite)
	facility = solcorp.helpers.add_facility_to_building(building, "Line 1")
	facility:getManufacturing()
	facility = solcorp.helpers.add_facility_to_building(building, "Line 2")
	facility:getManufacturing()

	-- Create a new site
	local site = solcorp.helpers.create_site("Cape Canaveral", 10, 10, true)

	solcorp.helpers.create_building("Manufacturing A", "Factory", 1, 1, site)
	solcorp.helpers.create_building("Storage Hall 1", "Storage Hall", 0, 0, site)
	solcorp.helpers.create_building("Main Launchpad", "Launch Complex", 1, 0, site)
	solcorp.helpers.create_building("North Building", "Office Building", 2, 0, site)
	local south_pad = solcorp.helpers.create_building("South Launchpad", "Launch Complex", 5, 5, site)

	local e = solcorp.entities.create()
	e:child_of(south_pad)
	e:getText().text = "South Launchpad"
	e:getVelocity().y = -20
	e:getTransform().relativePosition.y = -15
	e:getExpire().millis = 2000

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

	mod.target_stat = "prep-days"
	mod.multiplicative = 1.0
	mod.additive = 3
	solcorp.helpers.add_modifier(cracks, mod)

	-- Create a rocket prefab
	local rocket = solcorp.helpers.create_rocket_prefab("Falcon 1")
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Low Orbit", 6300)
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Polar Orbit", 5600)
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Transfer Orbit", 3300)
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Synchronous Orbit", 1300)
end

solcorp.script.handlers.on_init = on_init
solcorp.script.handlers.on_start = on_start
solcorp.script.handlers.on_frame = nil
solcorp.script.handlers.on_update = nil
