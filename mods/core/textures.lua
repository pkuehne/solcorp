-- Data-driven texture definitions for the core mod.
--
-- Returns an id-keyed map of textures the engine loads (via LuaDataFile) before
-- buildings, so building sprites can be validated against them. How a texture
-- is cut into tiles lives in tilesets.lua, not here (ADR 013).
--
-- `file` is relative to this mod's directory.
return {
	Buildings = { file = "buildings.png" },
	roads_asphalt = { file = "roads_asphalt.png" },
	roads_markings = { file = "roads_markings.png" },
}
