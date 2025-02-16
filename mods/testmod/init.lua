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
end

solcorp.script.handlers.on_init = on_init
solcorp.script.handlers.on_start = function()
	local info = solcorp.logging.info
	info("on_start called!")

	local comp = solcorp.entities.GameComponent()
	comp.day = 12
	info("Day: " .. comp.day)

	-- local site = solcorp.entities.get("Cape Canaveral")
	local site = solcorp.helpers.create_site("Cape Canaveral", 10, 10, true)

	solcorp.helpers.create_building("Manufacturing A", "Factory", 1, 1, site)
	solcorp.helpers.create_building("Storage Hall 1", "Storage Hall", 0, 0, site)
	solcorp.helpers.create_building("Main Launchpad", "Launchpad", 1, 0, site)
	solcorp.helpers.create_building("North Building", "Office Building", 2, 0, site)
	local south_pad = solcorp.helpers.create_building("South Launchpad", "Launchpad", 5, 5, site)
	local lp = south_pad:getLaunchpad()
	lp.max_weight.base = 5000

	local concrete = solcorp.helpers.create_effect("Better Concrete", site)
	local mod = Modifier:new()
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
