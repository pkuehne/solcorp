-- Data-driven texture definitions for the core mod.
--
-- Returns an id-keyed map of textures the engine loads (via LuaDataFile) before
-- buildings, so building sprites can be validated against them. Each value is a
-- table (not a bare path) so textures can grow fields later, e.g. tile size and
-- column count for tileset atlases.
--
-- `file` is relative to this mod's directory.
return {
	Buildings = { file = "buildings.png" },
}
