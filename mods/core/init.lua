local function on_init()
	-- init function
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

	-- Textures and building prefabs are now declared as data in textures.lua and
	-- buildings.lua, loaded and applied by the engine before on_start runs.

	-- Create a new site
	solcorp.helpers.create_site("Cape Canaveral", 10, 10, true)
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
