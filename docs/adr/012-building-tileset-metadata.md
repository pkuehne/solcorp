# 012 — Data-Driven Building Tileset Metadata

**Status:** Accepted

## Context

Buildings in SolCorp today render from a single `Sprite` — a clipped rect into a shared texture, set
imperatively via `building:setSprite(...)` ([helpers.cpp](../../src/modules/lua/helpers.cpp)). The
move to a tileable sprite set is not yet built. The first design for it — sketched in
[ADR 009 §8](/adr/009-build-mode-and-site-window.md) — used a fixed 9-slice convention: a building
occupies at minimum nine tiles (four corners, four edges, one centre), and a tile's integer index
directly encoded its position — index 4 was always "centre", and so on.

This held while every building shared one foundation tileset and every role had exactly one tile. It
stops holding as soon as buildings diverge, which they do:

- **Variants per role.** A manufacturing hall wants several interchangeable centre tiles so a large
  roof does not visibly repeat. "Index 4 = the centre tile" cannot express "any of these three centre
  tiles."
- **Different buildings, different needs.** A silo, an office, and a hall do not share a foundation.
  Roofs, wall treatments, and footprint shape differ. The set of tiles is per building type, not
  universal.
- **Features vs. embellishments.** Buildings carry *functional features* (entry door, cargo door,
  road connection) that gameplay must be able to locate, and *cosmetic embellishments* (HVAC units,
  forklift towers, vents) that are pure flavour. Both attach to specific surfaces under placement
  constraints.
- **State-driven tiles.** A launchpad roof tile should show a rocket only once rollout is complete; a
  powered building might glow. The same tile slot needs to render differently depending on building or
  world state.

The common failure is that *index* was being asked to mean two things at once: **where** a tile sits
(its 9-slice role) and **what** a tile is (a specific sprite with its own surface, mounts, and
conditions). Once a role can have multiple candidate tiles, and a tile can be conditional, position
can no longer be inferred from the number.

The roads/connector work (see [ADR 010](/adr/010-connector-tile-system.md)) established two patterns
this ADR builds on. First, a placed tile is rendered as **multiple layers**; ADR 010 draws those from
a fixed set of relationship slots (Base → Markings → Embellishments) precisely because their count is
fixed at three. Second, a site's tiles are modelled as **Flecs child entities** that a system walks —
the connector retiler collects a site's connector children to compute its layout. This ADR needs a
*variable* number of tiles and mounts per building, which the fixed-slot scheme cannot express, so it
extends the child-entity model: a placed building becomes the parent and its variant tiles and mounts
become child entities that the render system walks.

## Decision

Replace the fixed index-as-position scheme with a **flat, data-driven registry of tile definitions**.
Each building type registers a tileset: a list of self-describing `TileDef` entries. Index becomes an
opaque id into this table, not a positional code. Role becomes a field of the definition.

C++ owns the typed vocabulary (structs, enums, the predicate type); Lua prefabs populate instances.
This is the split the project already uses: `ConnectorVariant` is a C++ enum exposed to Lua via
`register_enum_table_lua` so mods write `ConnectorVariant.Straight` while C++ owns the values
([connector.h](../../src/modules/site/connector.h), [lua.h](../../src/modules/lua/lua.h)); component
values cross the boundary through `register_component_lua`. The same mechanism carries `TileDef`. It
also keeps the registry trivially moddable: mods append new definitions or override by id, consistent
with the last-writer-wins registry pattern established in [ADR 011](/adr/011-lua-mod-registry.md).

The `sprite` field of a `TileDef` follows the existing reference model — a clipped rect into a named
texture entity (`Textures::<name>`, as `clip_sprite_from_texture` and the connector tilesets already
do), not a raw pixel handle.

### Tile definition

Each entry in a building's tileset:

| Field | Meaning |
|---|---|
| `id` | Stable string id (`hall_centre_a`, `hall_launchpad`) |
| `sprite` | Reference into the spritesheet |
| `roles` | Set of 9-slice roles this tile may fill (`{Centre}`, `{Bottom,BL}`) |
| `surface` | `Roof` \| `SouthWall` \| `None` — what may mount here |
| `weight` | Relative selection weight among candidates for a role |
| `predicate` | Optional eligibility condition (`nil` = always eligible) |
| `mounts` | List of `MountRule` — features and embellishments allowed on this tile |
| `tags` | Gameplay/query tags (`launchpad`, `powered`) |

Role is a **field on each tile** (a tile self-declares "I can be a centre tile"), and the registry is
**flat** rather than pre-bucketed by role. Consumers query the registry for what they need: the build
system asks for a tile satisfying `Centre in roles` with a satisfied predicate, then weighted-random
picks among candidates; the render system asks what sprite and child layers a placed tile has;
placement logic reads `mounts` off the same definitions. One source of truth, every system reads its
own slice.

### Features and embellishments (unified mount rules)

Mount rules live on the tile definition — a mount is a property of the *surface* it attaches to, not
of the building globally. Functional features and cosmetic embellishments use the **same** struct,
distinguished by a `category` field:

| Field | Meaning |
|---|---|
| `kind` | `entry_door`, `hvac`, `rocket` |
| `category` | `Feature` (gameplay-queryable) \| `Embellishment` (cosmetic) |
| `required` | `entry_door` true; `hvac` false |
| `max_count` | Cap on instances |
| `min_spacing` | Tiles between instances |
| `predicate` | Optional condition (e.g. rocket only if rollout complete) |

Building-level constraints (e.g. `min_building_width`) live on the building definition. Gameplay code
queries only `Feature`-category mounts and never sees embellishments.

### Predicates

A predicate gates both tile eligibility (a launchpad swaps its sprite) and mount eligibility (a rocket
child appears). It is one shared type used in both places, so state-driven tiles and state-driven
embellishments use identical machinery.

**v1 is a single named flag** the building exposes (`rollout_complete`, `powered`). This is
deliberately the smallest thing that works. The extension path is deferred: additional named
predicates as needed (`has_stored_rocket`, `empty_office`, `no_upkeep`, `unpowered`, `meteor_damage`),
and eventually a Flecs-query-based predicate for compound conditions. Compound and expression
predicates are out of scope for v1.

### Placement and storage

Variant selection (which centre tile fills a given cell) and mount placement (which features and
embellishments go where) are resolved **once at construction** and **stored** as Flecs child entities
of the building. This:

- gives deterministic, stable visuals with no seed-and-reroll scheme — the child entities are designed
  to *be* the save state. (Caveat: there is no save/load system in the project yet; this leans on
  Flecs serialization being wired into a future one, so it is a forward-looking assumption rather than
  a current capability.)
- extends the child-entity model of [ADR 010](/adr/010-connector-tile-system.md) — where a site's
  connectors are its child entities — down one level: a building's variant tiles and mounts are *its*
  child entities, which the render system walks parent → child, adding building-as-parent on top of
  010's site-as-parent;
- means conditional tiles and mounts are *placed* once but their predicate is *evaluated at
  render/update time*, so a launchpad's rocket appears when `rollout_complete` flips without re-running
  placement.

## Consequences

**Positive**

- One source of truth per building type; build, render, and placement systems all read the same
  registry instead of sharing implicit index conventions.
- Multiple tiles per role (anti-repeat roofs) fall out naturally.
- Per-building-type foundations are supported without special-casing.
- Features vs. embellishments are cleanly separated; gameplay can locate doors without knowing about
  HVAC units.
- Launchpad rocket, powered glow, and damage states are not special cases — they are the predicate
  mechanism applied to tiles or mounts.
- Fully moddable: a flat registry of id'd definitions is append/override-friendly, matching the
  pattern from [ADR 011](/adr/011-lua-mod-registry.md).

**Negative / costs**

- More upfront data per building than a bare 9-index lookup; every tile now needs a definition.
  Mitigated by sensible Lua defaults so a minimal building stays terse.
- The fixed 9-slice index scheme planned in
  [ADR 009 §8](/adr/009-build-mode-and-site-window.md) is superseded: index becomes an opaque id and
  the corner/edge/centre role moves to a tile field. Code written against fixed positional indices
  (the `BuildingTileIndex` lookup envisaged there) must move to the query API; ADR 009 §8 should be
  read as refined by this ADR.
- A registry validation step is needed to catch malformed prefabs — including modded ones — at load
  rather than at render time (layered on the post-load validation pass of
  [ADR 011 §5](/adr/011-lua-mod-registry.md)): every required role must have at least one eligible
  tile, and `required` mounts must be placeable within their constraints.

**Deferred (explicitly out of scope for v1)**

- Compound/expression predicates and Flecs-query predicates.
- Re-running placement on building resize, upgrade, or tiering — the current model is one-shot at
  construction; if tiering later requires re-placement, placement logic must be made idempotent at
  that point.
- Per-building-type custom placement algorithms — v1 is one shared algorithm parameterised by data.

## Related

- [ADR 011](/adr/011-lua-mod-registry.md) — Lua Mod Registry and Prefab Override System (registry and
  merge pattern this ADR builds on; tile-registry validation extends ADR 011 §5).
- [ADR 010](/adr/010-connector-tile-system.md) — Connector Tile System (layered tile rendering and the
  site-child-entity model this ADR extends to building → child tiles/mounts).
- [ADR 009](/adr/009-build-mode-and-site-window.md) — Build Mode and Site Window (§8's fixed 9-slice
  index scheme is superseded by this data-driven tile registry; the build palette and placement kernel
  consume these definitions).
