local function on_start()
	local info = solcorp.logging.info
	info("on_start called!")
	local site = solcorp.entities.get("Sun::Earth::Cape Canaveral")
	if site == nil then
		info("Cape Canaveral not found!")
	else
		info("Cape Canaveral: " .. tostring(site:is_alive()))
	end

	-- Add a road connector
	local ConnectorVariant = solcorp.constants.ConnectorVariant
	for i = 0, 9 do
		solcorp.helpers.create_connector(
			"Road 3," .. i,
			ConnectorVariant.Straight,
			0,
			3,
			i,
			site,
			"roads_asphalt",
			"roads_markings"
		)
	end

	-- Add some buildings to the site
	solcorp.helpers.create_building("Manufacturing A", "factory", 1, 1, site)
	local storage = solcorp.helpers.create_building("Storage Hall 1", "storage_hall", 1, 0, site)
	local main_launchpad = solcorp.helpers.create_building("Main Launchpad", "launch_complex", 8, 5, site)
	solcorp.helpers.create_building("North Building", "office_building", 0, 6, site)

	-- Add some modifiers to the launchpad
	local mod = solcorp.components.Modifier:new()
	mod.target_stat = "max-weight"

	local concrete = solcorp.helpers.create_effect("Better Concrete", site)
	mod.multiplicative = 1.2
	solcorp.helpers.add_modifier(concrete, mod)

	local cracks = solcorp.helpers.create_effect("Cracks detected", main_launchpad)
	mod.multiplicative = 0.4
	solcorp.helpers.add_modifier(cracks, mod)

	local struts = solcorp.helpers.create_effect("Reinforcing Struts", site)
	mod.multiplicative = 1.0
	mod.additive = 500
	solcorp.helpers.add_modifier(struts, mod)

	-- Create a rocket
	local hall = storage:lookup("Hall 1")
	solcorp.helpers.create_rocket("Falcon 1 - Unit 001", "falcon_1", hall)

	-- Create a contract
	local contract = solcorp.helpers.create_contract(
		"Your first contract",
		"Angel Investors Inc.",
		"Launch your first satellite into low Earth orbit.",
		2 * 1000 * 1000,
		2 * 1000 * 1000
	)
	solcorp.helpers.create_contract_payload(contract, "Inaugural 1", 800, "Sun::Earth::Low Orbit")
end

solcorp.script.handlers.on_init = nil
solcorp.script.handlers.on_start = on_start
solcorp.script.handlers.on_frame = nil
solcorp.script.handlers.on_update = nil
