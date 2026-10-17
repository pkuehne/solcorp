-- Data-driven tileset definitions for the core mod.
--
-- Returns an id-keyed map of tilesets, each cutting a texture from textures.lua
-- into equal tiles numbered row-major from 0 (ADR 013). `tile_size` sets both
-- dimensions; `tile_width` / `tile_height` can be given for non-square tiles.
return {
	roads_asphalt = { texture = "roads_asphalt", tile_size = 32 },
	roads_markings = { texture = "roads_markings", tile_size = 32 },
}
