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

local get_random_contract_name
local get_random_company_name

local function on_start()
	local info = solcorp.logging.info
	info("on_start called!")

	local company = solcorp.entities.Company()
	company.name = "Solar Corporation"
	company.balance = 9 * 1000 * 1000

	local comp = solcorp.entities.Game()
	comp.day = 1

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

	-- Add a road connector
	for i = 0, 8 do
		solcorp.helpers.create_connector("Road 1," .. i, ConnectorVariant.Straight, 90, 3, i, site, "roads_asphalt",
			"roads_markings")
	end
	solcorp.helpers.create_connector("Road 1, 9", ConnectorVariant.DeadEnd, 270, 3, 9, site, "roads_asphalt",
		"roads_markings")

	-- Add some buildings to the site
	solcorp.helpers.create_building("Manufacturing A", "Factory", 1, 1, site)
	local storage = solcorp.helpers.create_building("Storage Hall 1", "Storage Hall", 1, 0, site)
	local main_launchpad = solcorp.helpers.create_building("Main Launchpad", "Launch Complex", 8, 5, site)
	solcorp.helpers.create_building("North Building", "Office Building", 0, 6, site)

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

	-- Create a rocket prefab
	local rocket = solcorp.helpers.create_rocket_prefab("Falcon 1")
	rocket:getRocket().cost.base = 5 * 1000 * 1000
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Low Orbit", 6300)
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Polar Orbit", 5600)
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Transfer Orbit", 3300)
	solcorp.helpers.add_target_orbit_to_rocket(rocket, "Sun::Earth::Synchronous Orbit", 1300)

	-- Create a rocket
	local hall = storage:lookup("Hall 1")
	solcorp.helpers.create_rocket("Falcon 1 - Unit 001", "Falcon 1", hall)

	-- Create a contract
	local contract = solcorp.helpers.create_contract(
		get_random_contract_name(),
		get_random_company_name(),
		"Launch your first satellite into low Earth orbit.",
		2 * 1000 * 1000,
		2 * 1000 * 1000
	)
	solcorp.helpers.create_contract_payload(
		contract,
		"Satellite " .. math.random(100, 10000),
		800,
		"Sun::Earth::Low Orbit"
	)
end

get_random_contract_name = function()
	local names = {
		"Launch Satellite",
		"Deploy Space Station Module",
		"Deliver Cargo Pod",
		"Transport Crew",
		"Repair Orbital Platform",
		"Conduct Space Experiment",
		"Resupply Space Station",
		"Test New Rocket",
		"Satellite Servicing Mission",
		"Space Tourism Flight",
	}
	return names[math.random(1, #names)]
end

get_random_company_name = function()
	local names = {
		"SpaceY",
		"Galactic Ventures",
		"Orbital Dynamics",
		"Stellar Freight",
		"CosmoCorp",
		"AeroSpace Inc.",
		"Nova Launch Systems",
		"Interstellar Logistics",
		"AstroTech Solutions",
		"Celestial Enterprises",
	}
	return names[math.random(1, #names)]
end

local function create_contracts()
	local info = solcorp.logging.info
	local contracts = solcorp.helpers.get_all_active_contracts()
	if #contracts >= 5 then
		return
	end

	if math.random() < 0.8 then
		return
	end
	info("Creating new contract...")
	local contract = solcorp.helpers.create_contract(
		get_random_contract_name(),
		get_random_company_name(),
		"Launch a satellite into low Earth orbit.",
		4 * 1000 * 1000,
		4 * 1000 * 1000
	)
	solcorp.helpers.create_contract_payload(
		contract,
		"Satellite " .. math.random(100, 10000),
		500,
		"Sun::Earth::Low Orbit"
	)
	info("Contract created with ID: " .. contract:id())
end

local function on_update()
	-- update function
	-- print("on_update called!")
	create_contracts()
end

solcorp.script.handlers.on_init = on_init
solcorp.script.handlers.on_start = on_start
solcorp.script.handlers.on_frame = nil
solcorp.script.handlers.on_update = on_update
