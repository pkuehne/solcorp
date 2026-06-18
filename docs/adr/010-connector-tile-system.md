# 010 — Connector Tile System

**Status:** Accepted

## Context

Sites need a way to render network tiles — roads on launch sites, and in future, structural trusses
on space stations and tunnels in lunar colonies. All three share the same structural idea: a
directional tile that joins adjacent tiles of the same kind, drawn from a spritesheet that encodes
the join topology as a variant index.

Naming the concept after roads would lock future code into Earth-surface assumptions. The same
rendering and autotiling logic applies unchanged to trusses and tunnels, so the shared name matters.

Texture artists also want to layer visual detail without duplicating geometry: bare asphalt is one
layer, lane markings another, optional embellishments (cracks, patches, oil stains) a third. These
layers are separate spritesheets and can be omitted independently.

## Decision

### Terminology

The system is called **connectors** throughout the codebase (`ConnectorTile`, `connector.h`,
`registerConnectors`). "Road", "truss", and "tunnel" are site-type-specific labels for the same
ECS concept.

### Variant enum

Five topologies cover all join configurations for an orthogonal grid, stored as a `uint8_t` enum:

```cpp
enum class ConnectorVariant : uint8_t {
  Straight  = 0,
  Corner    = 1,
  TJunction = 2,
  Crossing  = 3,
  DeadEnd   = 4,
};
```

The enum value maps directly to a position in a 3×3 spritesheet atlas
(`col = value % 3`, `row = value / 3`), so adding a new variant only requires adding an entry here
and a corresponding column in the atlas. A single `rotation_deg` field on the component handles
all orientations of each topology; the engine applies it to every layer uniformly via
`SDL_RenderCopyExF`.

### Autotiling — variant and rotation are derived, not authored

In normal play the variant and rotation of a connector are **not** authored per tile; they are
derived from the tile's orthogonal **connector** neighbours by a pure kernel, so neither the player
nor build-mode placement (ADR 009) ever picks them. Only Lua seed scripts may pass explicit values,
and even those are overwritten the first time the owning site is relaid out (ADR 009 §6).

The four orthogonal neighbours form a 4-bit mask (N/E/S/W, in clockwise order) that maps to a
variant:

| neighbours | variant |
|---|---|
| 0 | isolated — left as authored |
| 1 | DeadEnd |
| 2 opposite (N+S / E+W) | Straight |
| 2 adjacent | Corner |
| 3 | TJunction |
| 4 | Crossing |

`rotation_deg` is found by rotating each variant's rotation-0 **base orientation** clockwise
(N→E→S→W) until its connection set matches the mask. The base orientations are fixed by the
spritesheet artwork:

| variant | connects at rotation 0 |
|---|---|
| Straight | East + West (horizontal) |
| Corner | East + South |
| TJunction | North + East + South (closed side faces West) |
| DeadEnd | East |

Straight and DeadEnd are additionally pinned by the existing Lua seed (a vertical road uses
rotation 90; a north-facing dead-end uses rotation 270), which the kernel reproduces.

The kernel (`computeConnectorTiling(mask)`) is a pure free function with no world dependency,
unit-tested across all sixteen masks. The system that applies it — collecting a site's connectors,
computing each tile's mask, and writing back `variant`/`rotation_deg` — is triggered by the site
relayout flag (`SiteNeedsRelayout`) described in ADR 009 §6, so it runs only when a site changes,
never every frame.

### Fixed layer slots via Flecs relationships

Rather than storing tileset entity IDs directly in the component struct (which would embed a world
pointer and break serialisation), tilesets are referenced via Flecs relationships:

```
entity --TilesetBase--> texture_entity
entity --TilesetMarkings--> texture_entity
entity --TilesetEmbellishments--> texture_entity
```

The render system calls `e.target<TilesetBase/Markings/Embellishments>()` at runtime. A missing
relationship (invalid target) means that layer is skipped — no special sentinel value needed.
Layers are always drawn in the order Base → Markings → Embellishments.

This pattern follows the same principle as other entity-to-entity references in the project: use
Flecs relationships, not raw IDs in component fields.

### SDL encapsulation

All SDL rendering calls stay inside `modules/engine/`. The connector system calls `renderTile()`
declared in [render.h](../../src/modules/engine/render.h), which handles source rect calculation,
destination mapping, and rotation. `connector.cpp` has no direct SDL dependency.

### Game asset textures

Textures shipped with the game (as opposed to mod textures) live in `assets/textures/` and are
loaded automatically at engine startup in `registerRender`. Every `.png` in that directory becomes
a child of the `Textures` world entity, addressable as `Textures::<stem>`. Mods load their own
textures via `create_texture`, which reads from `mods/<modname>/`; mods have no API to reach
`assets/textures/`.

### Lua exposure

`ConnectorVariant` is registered as a global enum table via `register_enum_table_lua` so Lua mods
can write `ConnectorVariant.Straight`. `ConnectorTile` is registered via `register_component_lua`
for get/set/has/remove access. Relationship setup (tileset binding) is handled inside
`create_connector` in `helpers.cpp` because Lua has no direct relationship API.

## Consequences

- Adding a new connector type (truss, tunnel) requires: a site-type label, an appropriately
  structured spritesheet in `assets/textures/`, and a `Buildable` relationship on the site
  (ADR 009). No changes to `ConnectorTile`, the render system, or the autotiler.
- The layer slots are fixed at three. A connector that needs four layers would require a new
  relationship tag and a render-system change.
- `rotation_deg` is shared across all layers. Per-layer rotation is not supported; all layers of
  a tile are treated as a single sprite.
- Asset textures are loaded once at startup. Hot-reloading is not supported.

## Alternatives Considered

- **Name the system "roads"**: Rejected — would require renaming when stations or colonies are
  added, and implies Earth-surface assumptions in code that is site-type-agnostic.
- **Store tileset entity IDs directly in the component struct**: Rejected — `flecs::entity` in a
  component struct embeds a world pointer and is not serialisable; raw `ecs_entity_t` is
  technically acceptable but relationships are the idiomatic Flecs pattern.
- **Single combined spritesheet with all layers pre-composited**: Rejected — artists cannot
  independently vary the markings layer (e.g. different road paint per region) or apply
  embellishments selectively.
- **Per-layer rotation**: Not needed for the current topology variants; all orientations are
  achieved by rotating all layers together.
