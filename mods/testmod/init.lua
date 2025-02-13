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
	info("Day: " .. comp.day)
	comp.day = 321

	local site = solcorp.entities.get("Cape Canaveral")
	local south_pad = solcorp.entities.create("South Launchpad")
	south_pad:is_a("Buildings::Launchpad")
	south_pad:child_of(site)
	local location = south_pad:getSiteLocation()
	location.x = 5
	location.y = 5
end
