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
	comp.day = 1
	info("Day: " .. comp.day)

	local site = solcorp.entities.get("Cape Canaveral")
	local south_pad = solcorp.entities.create("South Launchpad (Heavy)")
	south_pad:is_a("Buildings::Launchpad")
	south_pad:child_of(site)
	local location = south_pad:getSiteLocation()
	location.x = 5
	location.y = 5
	local lp = south_pad:getLaunchpad()
	info("Launchpad max weight: " .. lp.max_weight:value())
	lp.max_weight.base = 5000

	local max_height = Stat:new()
	max_height.base = 100
	info("Max height: " .. max_height:value())
end
