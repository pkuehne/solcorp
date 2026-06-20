-- Data-driven rocket prefab definitions for the core mod.
--
-- Returns an id-keyed map the engine loads (via LuaDataFile) and applies to
-- the ECS. Each entry describes a rocket prefab: its display name, build cost
-- and the orbits it can lift to (with the maximum payload mass, in kg, for
-- each). The key is the stable prefab id referenced by
-- solcorp.helpers.create_rocket; the optional `name` is the player-visible
-- label (defaults to the id).
--
-- See ADR 011 (Lua mod registry).
return {
	falcon = {
		name = "Falcon",
		cost = 5 * 1000 * 1000,
		orbits = {
			{ target = "Sun::Earth::Low Orbit", mass = 6300 },
			{ target = "Sun::Earth::Polar Orbit", mass = 5600 },
			{ target = "Sun::Earth::Transfer Orbit", mass = 3300 },
			{ target = "Sun::Earth::Synchronous Orbit", mass = 1300 },
		},
	},
}
