-- Building tile definitions for the core mod, keyed by building id.
--
-- Each entry names the tileset its indices refer to (see tilesets.lua); every
-- other table is a tile, filling one or more 9-slice roles (ADR 012).
return {
	factory = {
		tileset = "factory",
		roof_top_left = { index = 1, roles = { "TopLeft" }, surface = "Roof" },
		roof_top = { index = 2, roles = { "Top" }, surface = "Roof" },
		roof_top_right = { index = 3, roles = { "TopRight" }, surface = "Roof" },
		roof_left = { index = 4, roles = { "Left" }, surface = "Roof" },
		roof = { index = 5, roles = { "Centre" }, surface = "Roof" },
		roof_right = { index = 6, roles = { "Right" }, surface = "Roof" },
		wall_left = { index = 7, roles = { "BottomLeft" }, surface = "SouthWall" },
		wall = { index = 8, roles = { "Bottom" }, surface = "SouthWall" },
		wall_right = { index = 9, roles = { "BottomRight" }, surface = "SouthWall" },
	},
	launch_complex = {
		tileset = "launchpad",
		roof_top_left = { index = 1, roles = { "TopLeft" }, surface = "SouthWall" },
		roof_top = { index = 2, roles = { "Top" }, surface = "SouthWall" },
		roof_top_right = { index = 3, roles = { "TopRight" }, surface = "SouthWall" },
		roof_left = { index = 4, roles = { "Left" }, surface = "SouthWall" },
		roof = { index = 5, roles = { "Centre" }, surface = "SouthWall" },
		roof_right = { index = 6, roles = { "Right" }, surface = "SouthWall" },
		wall_left = { index = 7, roles = { "BottomLeft" }, surface = "SouthWall" },
		wall = { index = 8, roles = { "Bottom" }, surface = "SouthWall" },
		wall_right = { index = 9, roles = { "BottomRight" }, surface = "SouthWall" },
	},
	office_building = {
		tileset = "office",
		roof_top_left = { index = 1, roles = { "TopLeft" }, surface = "Roof" },
		roof_top = { index = 2, roles = { "Top" }, surface = "Roof" },
		roof_top_right = { index = 3, roles = { "TopRight" }, surface = "Roof" },
		roof_left = { index = 4, roles = { "Left" }, surface = "SouthWall" },
		roof = { index = 5, roles = { "Centre" }, surface = "SouthWall" },
		roof_right = { index = 6, roles = { "Right" }, surface = "SouthWall" },
		wall_left = { index = 7, roles = { "BottomLeft" }, surface = "SouthWall" },
		wall = { index = 8, roles = { "Bottom" }, surface = "SouthWall" },
		wall_right = { index = 9, roles = { "BottomRight" }, surface = "SouthWall" },
	},
	storage_hall = {
		tileset = "storage",
		roof_top_left = { index = 1, roles = { "TopLeft" }, surface = "Roof" },
		roof_top = { index = 2, roles = { "Top" }, surface = "Roof" },
		roof_top_right = { index = 3, roles = { "TopRight" }, surface = "Roof" },
		roof_left = { index = 4, roles = { "Left" }, surface = "Roof" },
		roof = { index = 5, roles = { "Centre" }, surface = "Roof" },
		roof_right = { index = 6, roles = { "Right" }, surface = "Roof" },
		wall_left = { index = 7, roles = { "BottomLeft" }, surface = "SouthWall" },
		wall = { index = 8, roles = { "Bottom" }, surface = "SouthWall" },
		wall_right = { index = 9, roles = { "BottomRight" }, surface = "SouthWall" },
	},
}
