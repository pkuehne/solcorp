# 013 — Tilesets

**Status:** Proposed

## Context

Every piece of tile art currently refers straight to a texture and works out its own position in it:

- **Buildings** carry a `Sprite` with an explicit pixel rect (`x`, `y`, `width`, `height`) into a
  named texture, written by hand in `buildings.lua`.
- **Connectors** (ADR 010) point their layer relationships (`TilesetBase`, `TilesetMarkings`,
  `TilesetEmbellishments`) at a `Texture` entity, and `connector.cpp` derives the source rect from a
  hard-coded `TILE_SIZE` and the strip layout.
- **Building tiles** (ADR 012) were specified the same way: a `TileDef`'s `sprite` is a clipped rect
  into a named texture.

So knowledge of the grid — tile size, how many columns, which index sits where — is repeated in data
files and C++ constants instead of being stated once next to the image. Putting it on `Texture`
instead was considered and rejected (#231): a texture is just pixels, and one image can be cut up in
more than one way.

Most 2D engines and tools separate these concerns into three levels: the image, a **tileset** that
describes how the image is cut into equal tiles, and **tiles** that refer to a tileset by index
(Tiled's `.tsx`, Godot's `TileSet`, Unity's sliced sprite sheets, Aseprite's tileset export).

## Decision

### Texture → Tileset → Tile

- **Texture** — unchanged: the loaded image, owned by a mod (ADR 010 §Game asset textures).
- **Tileset** — a new entity under `Tilesets::<id>` that refers to its texture through a
  relationship and holds the grid geometry:

  ```cpp
  /// Tileset --TilesetTexture--> Textures::<name>
  struct TilesetTexture {};

  struct Tileset {
    int tile_width = 0;
    int tile_height = 0;
    int columns = 0; // derived from the texture width when the tileset is created
  };
  ```

  `columns` is computed from the texture width at creation, not authored, so the data and the image
  cannot disagree.
- **Tile** — anything drawn from a tileset holds only a tile **index** and a relationship to the
  tileset. Index → source rect is one pure function, unit-tested in isolation:

  ```cpp
  SDL_Rect tileRect(const Tileset &tileset, uint32_t index);
  ```

Indices are raw positions in the image, row-major from 0. Tilesets exported by Aseprite reserve index
0 for the empty tile; that is a property of the art, not of the `Tileset`, so consumers that know their
layout (the connector strip, ADR 010) account for it themselves.

### Declared as mod data

Tilesets are a new mod-registry category (ADR 011), `tilesets.lua`, merged in load order and applied
after textures:

```lua
return {
	roads_asphalt = { texture = "roads_asphalt", tile_size = 32 },
	buildings = { texture = "Buildings", tile_size = 32 },
}
```

`tile_size` covers square tiles; `tile_width` / `tile_height` can be given for non-square tiles.
Validation follows ADR 011 §5: a tileset whose texture is missing, or whose tile size does not divide
the texture, is logged with its provenance and skipped.

### Consumers

- **Connectors** — the layer relationships target a `Tileset` instead of a `Texture`;
  `TILE_SIZE` and the rect maths in `connector.cpp` go away.
- **Buildings** — the per-building pixel rect in `buildings.lua` becomes `{ tileset, index }`.
- **Building tiles** — a `TileDef`'s `sprite` (ADR 012) is a tile index into the building's tileset.
- **`Sprite`** — stays for art that is not on a grid. Its `texture` member becomes a relationship
  (#232), which is a separate change.

## Consequences

- Grid geometry is stated once per image, next to it, in data; tile users only say *which* tile.
- One tested function owns index → rect for every tiled thing in the game.
- Mods can retile or swap art by overriding a `tilesets.lua` entry without touching buildings or
  connectors.
- One more loading step and registry category; each image used as tiles needs a `tilesets.lua`
  entry.
- Only uniform grids are supported. Margins, spacing between tiles, and per-tile metadata (Tiled
  properties, Aseprite slice data) are out of scope until art needs them.
- Rendering stays on SDL's 2D renderer. A later move to shaders changes how a texture is stored and
  drawn, not the tileset geometry.

## Alternatives Considered

- **Tile size and columns on `Texture`**: rejected — mixes "the pixels" with "one way of cutting
  them up", and stops an image from being cut up more than one way.
- **Keep explicit pixel rects per tile**: rejected — repeats grid knowledge in every definition and
  makes re-exported art error-prone.
- **Authored `columns`**: rejected — derivable from the texture width, so authoring it only adds a way
  to be wrong.
- **Import Tiled `.tsx` / Aseprite JSON metadata directly**: deferred — useful once tiles carry
  per-tile data; a uniform grid needs only a tile size.

## Related

- [ADR 010](/adr/010-connector-tile-system.md) — connector layers will target tilesets.
- [ADR 011](/adr/011-lua-mod-registry.md) — `tilesets` is a new registry category.
- [ADR 012](/adr/012-building-tileset-metadata.md) — `TileDef.sprite` becomes a tile index.
