-- Data-driven building prefab definitions for the core mod.
--
-- Returns an id-keyed map the engine loads (via LuaDataFile) and applies to the
-- ECS. Each entry describes a building prefab: its display name, the sprite it
-- clips from a named texture (see textures.lua), and its facilities.
--
-- See ADR 011 (Lua mod registry) and ADR 012 (building tileset metadata).
return {
	launch_complex = {
		name = "Launch Complex",
		sprite = { texture = "Buildings", x = 0, y = 96, width = 32, height = 32 },
		facilities = {
			{ name = "Launchpad A", type = "Launchpad" },
		},
	},

	office_building = {
		name = "Office Building",
		sprite = { texture = "Buildings", x = 0, y = 32, width = 32, height = 32 },
		facilities = {
			{ name = "Floor 1", type = "Office" },
		},
	},

	storage_hall = {
		name = "Storage Hall",
		sprite = { texture = "Buildings", x = 0, y = 64, width = 32, height = 32 },
		facilities = {
			{ name = "Hall 1", type = "Storage" },
		},
	},

	factory = {
		name = "Factory",
		sprite = { texture = "Buildings", x = 0, y = 64, width = 32, height = 32 },
		facilities = {
			{ name = "Line 1", type = "Manufacturing" },
			{ name = "Line 2", type = "Manufacturing" },
		},
	},
}
