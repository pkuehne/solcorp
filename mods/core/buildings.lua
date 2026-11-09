-- Data-driven building prefab definitions for the core mod.
--
-- Returns an id-keyed map the engine loads (via LuaDataFile) and applies to the
-- ECS. Each entry describes a building prefab: its display name, the tile it is
-- drawn with (see tilesets.lua), and its facilities.
--
-- See ADR 011 (Lua mod registry) and ADR 012 (building tileset metadata).
return {
	launch_complex = {
		name = "Launch Complex",
		tile = { tileset = "buildings", index = 3 },
		facilities = {
			{ name = "Launchpad A", type = "Launchpad" },
		},
	},

	office_building = {
		name = "Office Building",
		tile = { tileset = "buildings", index = 1 },
		facilities = {
			{ name = "Floor 1", type = "Office" },
		},
	},

	storage_hall = {
		name = "Storage Hall",
		tile = { tileset = "buildings", index = 2 },
		facilities = {
			{ name = "Hall 1", type = "Storage" },
		},
	},

	factory = {
		name = "Factory",
		tile = { tileset = "buildings", index = 2 },
		facilities = {
			{ name = "Line 1", type = "Manufacturing" },
			{ name = "Line 2", type = "Manufacturing" },
		},
	},
}
