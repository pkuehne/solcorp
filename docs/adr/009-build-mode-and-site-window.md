# 009 — Build Mode and Site Window

**Status:** Accepted

## Context

Construction today works by scattering ghost tiles. `systemUpdateConstructionSiteLocations`
([site_construction.cpp](../../src/modules/site/site_construction.cpp)) walks the site grid and
places a `ConstructionSite` entity on every empty cell orthogonally adjacent to a building or
connector (or the centre cell if the site is empty). Clicking a ghost opens the
`ConstructionSiteWindow`, which lists every building prefab as a button
([construction_window.cpp](../../src/modules/site/construction_window.cpp)); choosing one
instantiates it at that cell and re-runs the scatter.

This works for "drop one building next to an existing one" but does not generalise to the
direction the game is moving:

- **Roads are networks, not point placements.** Connector tiles
  ([connector.h](../../src/modules/site/connector.h)) already exist and can be created from Lua
  ([init.lua](../../mods/core/init.lua)), but a ghost-per-adjacent-empty-cell model cannot express
  dragging a road across multiple tiles, and the player should not hand-pick the tile variant and
  rotation the way the Lua seed currently does.
- **Multi-tile buildings** cannot be validated against a single `SiteLocation`. A 2×2 footprint
  needs a region check.
- **Other site types** (station modules, moon habitats) need different placement rules. The scatter
  logic hardcodes "adjacent to a building," so it cannot be reused.

The player also has nowhere to see information *about a site* — only individual buildings have a
detail window.

## Decision

Replace the scatter-ghost construction flow with an explicit **build mode** driven from a new
**Site Window**, backed by a single testable placement-validation function.

### 1. The site declares what is buildable

Buildable categories are data on the site (or its site-type prefab), not hardcoded in the window.
A site exposes its buildable set so the palette can query it:

```cpp
/// @brief A category of thing that may be built on a site (relationship target).
/// Site --Buildable--> Prefabs::Buildings
/// Site --Buildable--> Prefabs::Roads
struct Buildable {};
```

An Earth site is `Buildable` of `Prefabs::Buildings` and `Prefabs::Roads`; a future station is
`Buildable` of `Prefabs::Modules`. The Site Window renders one palette section per buildable
target. This relationship is the seam that lets stations and moons reuse the whole system.

> Note: ADR 011 also defines a per-prefab `buildable` *boolean* (does this specific prefab appear in
> the player build UI). That is a finer axis than this relationship and composes with it: the
> `Buildable` relationship here decides which *categories* a site accepts and therefore which palette
> *sections* exist; ADR 011's flag decides whether an *individual* prefab within an accepted category
> is offered to the player. See [ADR 011 §3](/adr/011-lua-mod-registry.md).

### 2. Footprint is chosen at placement time; a tag marks the fixed-size exceptions

Most buildings have **no fixed footprint**. The player drags out a rectangle and the building is
sized to fit — minimum 2×2, maximum bounded only by the site dimensions. A minority of buildings
(entry gate, substation) are dedicated 1×1 objects. The two cases reduce to a single distinction, so
a marker tag carries it; the *actual* footprint is stored per-instance once placed:

```cpp
/// @brief Marks a prefab as a single 1×1 object placed by one click,
/// rather than a drag-sized multi-tile building. Absence => draggable 2×2-and-up.
struct FixedSize {};

/// @brief The realised size of a placed instance (origin is SiteLocation).
struct Footprint {
  uint8_t w = 1;
  uint8_t h = 1;
};
```

A gate carries `FixedSize`; a generic hangar does not and is dragged out from 2×2 up to the site
bounds. The minimum (2×2) is a build-mode constant, not per-prefab data; the maximum is always the
site. Connectors (roads) are their own 1×1 placement path (see §7) and don't need the tag. Occupancy
and validation are written against `Footprint` regions from the start.

> Terminology: the network-tile concept is **connector** in code (ADR 010); "road" is the
> Earth-site label. Code identifiers below use `Connector`; prose says "road (connector)".

### 3. Build mode is ECS state, not a window

A singleton component holds the active tool:

```cpp
enum class PlacementKind : uint8_t { Building, Connector };

struct BuildMode {
  flecs::entity prefab = flecs::entity::null(); // what is being placed
  PlacementKind kind   = PlacementKind::Building;
  SiteLocation  dragStart{};                    // anchor for drag (connector run / building rect)
  bool          dragging = false;
};
```

Both kinds are placed by dragging, but the **geometry differs fundamentally**, which is why they are
two distinct Actions (§4): a building rubber-bands a 2D **rectangle** (2×2 up to site bounds) and
commits as **one multi-tile entity**; a connector traces a 1-wide **run** — always 1×N or N×1, never a
rectangle (you would never drag out a 4×4 road) — and commits as **N separate 1×1 entities** that then
autotile (§6). A single click on a `FixedSize` prefab places it at one cell.

Selecting a palette entry sets `BuildMode`. A ghost cursor entity (reusing the existing
`ConstructionSite` sprite) follows the mouse cell and renders valid/invalid (green/red). Right-click
or Esc clears `BuildMode`. When `BuildMode` is absent, clicks behave as today (open building / site
windows).

### 4. Placement commits through an Action; the spatial rules are a pure kernel underneath

Placing a building or connector satisfies all three of ADR 004's criteria — it mutates world state, it
has preconditions that can fail and the UI must say why, and the user builds up a draft (selected
prefab + dragged footprint). So the **commit is an `IAction`**, consistent with every other world
mutation in the codebase, rather than a one-off free function called from input code.

Buildings and connectors are **two separate Actions**, because their commits are structurally
different operations, not parameterisations of one (see §3): a building commits a single multi-tile
entity validated as a region; a connector commits a 1-wide run as N separate 1×1 entities validated
per cell, then autotiled. Collapsing them behind one Action with a `kind` switch would mean one
`execute()` branching between "create one entity" and "create a loop of entities" and one `validate()`
applying region-vs-path rules — exactly the implicit coupling ADR 004 warns against.

```cpp
struct PlaceBuildingAction : IAction {
  flecs::entity prefab;
  flecs::entity site;
  SiteLocation  origin;
  Footprint     footprint;        // realised rectangle from the drag (≥ 2×2, or 1×1 if FixedSize)
  ValidationResult validate(const flecs::world &) const override;
  void execute(flecs::world &) override;     // instantiates one multi-tile building
};

struct PlaceConnectorAction : IAction {
  flecs::entity prefab;
  flecs::entity site;
  SiteLocation  from, to;         // the 1-wide run (from == to for a single tile)
  ValidationResult validate(const flecs::world &) const override;
  void execute(flecs::world &) override;     // instantiates one 1×1 connector per valid cell, then autotiles
};
```

The **draft** lives in `BuildMode` (§3) — that *is* the draft state, the way `LaunchWindow::draftPlan`
holds a launch draft. A building `execute()` instantiates one entity; a connector `execute()`
instantiates the run's cells (binding tileset relationships from the prefab, per ADR 010) and flags
the site dirty so the autotiler (§6) recomputes variants.

The spatial rules themselves stay a **pure, unit-testable kernel** that `validate()` delegates to. It
reads an occupancy grid, not the live world, so tests construct grids directly without a world
harness (the project convention of extracting logic into named free functions):

```cpp
enum class PlacementResult : uint8_t {
  Ok,
  OutOfBounds,
  Occupied,
  TooSmall,                  // dragged rectangle below the 2×2 minimum
  NotConnectedToRoad,        // building has no adjacent connector
  ConnectorNotConnected,     // connector has no adjacent connector or building
};

PlacementResult canPlace(const OccupancyGrid &grid, PlacementKind kind,
                         SiteLocation origin, Footprint footprint);
```

`validate()` builds/holds an `OccupancyGrid` from the site, calls `canPlace`, and maps the
`PlacementResult` to a `ValidationResult` message for the UI (re-validating defensively at the start of
`execute()`, per ADR 004).

Why keep `canPlace` separate rather than inlining the logic in `validate()` like `ScheduleLaunchAction`
does: the **ghost preview** has to evaluate validity *every frame*, across many cells (a whole drag
rectangle or connector run), purely to colour the overlay green/red. That is read-only rendering, not a
world operation, so the ghost system calls `canPlace` directly against a cached grid — no Action churn
per frame. The Action is constructed and run only on commit (mouse-up). Both paths share one kernel, so
the preview can never disagree with what the commit will accept.

Connectivity and size rules (in `canPlace`):
- **Building** — a draggable building's footprint must be at least 2×2 (`TooSmall` otherwise) and fit
  the site; a `FixedSize` building is 1×1. Every cell must be in-bounds and empty, and at least one
  cell of the footprint perimeter must be orthogonally adjacent to a connector.
- **Connector** — the cell must be in-bounds and empty, and orthogonally adjacent to another connector
  or a building edge. The first tile on an empty site is allowed unconditionally (bootstrap),
  mirroring the current "centre cell" special case.

`canPlace` takes the realised `Footprint`; the min-size check is a small guard layered on top by the
caller so the core region/connectivity logic stays size-agnostic and easy to test.

### 5. Occupancy grid is lifted out of the scatter system

The transient `std::vector<LocationInfo>` built in `systemUpdateConstructionSiteLocations` becomes a
reusable value type built from a site's children:

```cpp
struct OccupancyGrid {
  uint8_t width, height;
  std::vector<LocationInfo> cells; // Empty / Building / Connector
};

OccupancyGrid buildOccupancyGrid(flecs::entity site);
```

Validation and ghost rendering both read it. It is rebuilt when the site changes — keyed off the
site dirty flag, renamed here from `ConstructionSiteNeedsUpdating` to `SiteNeedsRelayout` (§6) — and,
while in build mode, whenever needed for the ghost.

### 6. Connector autotiling — the engine derives variant and rotation

The connector tile model — the `ConnectorVariant` enum, the single `rotation_deg` field, the 3×3
atlas mapping, and the three-layer tileset relationships — is defined by **ADR 010**, which also
specifies the derivation **kernel** (the neighbour-mask → variant mapping and the base orientations
from which rotation is computed). This section covers how that kernel is *triggered* and *applied*.

**Trigger — a single site relayout flag.** A site whose layout changed — a connector or building
placed (§4), `create_connector` called from Lua, or a tile deleted (demolition, later) — is tagged
`SiteNeedsRelayout`. This is the renamed `ConstructionSiteNeedsUpdating`: the same site-level dirty
flag, repurposed to mean "recompute this site's derived layout state." A single `ValidatePhase`
system matches only flagged sites, so it runs on change, never every frame.

**Retile system** (`connector.cpp`). For each flagged site it gathers the site's connector children
into a set of occupied connector cells, builds each tile's N/E/S/W neighbour mask (only other
connectors count toward the join, not buildings), computes `{variant, rotation_deg}` via the ADR 010
kernel, and writes it back.

**Coexistence with the scatter system.** Until the scatter system is removed (Phase 3), it remains the
*owner* of the tag's lifecycle: it still clears `SiteNeedsRelayout` at the end of its pass. The
autotiler is therefore registered *before* the scatter system in `ValidatePhase` and deliberately does
**not** clear the tag — it runs first, retiles, and leaves the scatter system to clear. When the
scatter system is deleted in Phase 3, the autotiler becomes the sole consumer and takes over clearing
the tag.

The Lua `create_connector` API keeps explicit variant/rotation for scripted seeding, but it also flags
the owning site `SiteNeedsRelayout`, so the autotiler corrects those tiles on the next relayout pass —
the explicit values are advisory. Interactive placement never asks the player to choose.

This system depends only on the relayout tag and a site's connector children — not on `BuildMode`,
the Site Window, or `canPlace` — so it can be implemented independently of the other build-mode
phases.

### 7. Click-drag drawing

`MouseDown` records `dragStart`; while held, the ghost previews from `dragStart` to the current
cell; `MouseUp` commits. The preview shape depends on `PlacementKind`:

- **Connector** — a 1-wide run of cells (straight or L-shaped) from anchor to cursor; each cell
  validated by `canPlace`, only the valid ones placed, then the autotiler (§6) fixes variants.
- **Variable building** — a **rectangle** spanning anchor→cursor, clamped to a 2×2 minimum and the
  site bounds; placed as a single multi-tile instance if the whole region validates. Below the
  minimum it shows the `TooSmall` invalid state.
- **`FixedSize` prefab** — a single click places one 1×1 instance; no drag.

This needs a current-mouse-cell each frame, so a `MouseMove`/hover-cell signal is added alongside the
existing `MouseDown`/`MouseUp` ([input.h](../../src/modules/engine/input.h)).

### 8. Multi-tile buildings are 9-sliced from a tilesheet

> Refined by ADR 012: the fixed corner/edge/centre index scheme described in this section is
> generalised into a data-driven tile registry (roles become tile fields; a role may have multiple
> candidate tiles). The 9-slice *roles* survive; the *fixed index → position* mapping does not. Read
> this section together with [ADR 012](/adr/012-building-tileset-metadata.md).

A variable building's rectangle is rendered as a 9-slice: four corner tiles, four edge tiles
(stretched/tiled along each run), and a fill tile for the interior. The building prefab references a
9-slice tilesheet (corner/edge/centre source rects) rather than a single `Sprite`. A 2×2 minimum
guarantees every building has all four corners; a 1×N building (if ever allowed) would be a
degenerate case — disallowed by the 2×2 floor. Fixed single-tile buildings (gate, substation) keep a
plain `Sprite` and opt out of 9-slicing. Connectors are unaffected — they use the connector tile
model (ADR 010), not 9-slicing.

This is the reason the building footprint is a placement-time rectangle rather than a fixed prefab
size: the art scales with the footprint instead of the prefab dictating one size.

### 9. Ownership and module boundaries

Per the project rule that all UI windows live in `modules/window/`:

| Concern | Location |
|---|---|
| Site Window (info + palette) | `modules/window/site_window.{h,cpp}` |
| `PlaceBuildingAction` / `PlaceConnectorAction` (commit) | `modules/site/` (implement `IAction`) |
| `canPlace` kernel, occupancy grid, autotiling | `modules/site/` (pure free functions) |
| `BuildMode` singleton (draft) + ghost cursor | `modules/site/` state, rendered via engine overlay |
| Ghost / drag overlay rendering (reads `canPlace`) | `modules/engine/render` (or thin build overlay) |

`BuildMode` is the shared state the input, render, and site systems all read.

## Consequences

- `systemUpdateConstructionSiteLocations` and the scatter `ConstructionSite` ghosts are retired
  entirely. The `ConstructionSite` *prefab/sprite* is kept and repurposed as the build-mode ghost
  cursor.
- The site dirty flag `ConstructionSiteNeedsUpdating` is renamed `SiteNeedsRelayout` to reflect its
  broader meaning ("recompute this site's derived layout state"). The connector autotiler (§6) and the
  scatter system consume it side by side until the latter is removed in Phase 3, after which the
  autotiler is its sole consumer.
- The `ConstructionSiteWindow` (palette-as-popup-on-a-cell) is replaced by the Site Window palette.
- Placement rules become unit-tested in isolation, decoupled from rendering and from the live world.
- Sites gain a first-class info window.
- New input signal (hover cell / mouse move) is required.
- The Lua connector API keeps manual variant/rotation, but the autotiler overrides it whenever the
  owning site is relaid out — including the seed site at startup — so seeded and player-built
  connectors stay consistent without rewriting the Lua seeds.

## Alternatives Considered

- **Keep the scatter model, add a road sub-type.** Rejected: cannot express drag-drawing or
  network connectivity, and still hardcodes Earth-building assumptions.
- **Build mode as a modal window with a grid widget.** Rejected: placement should happen on the
  real map with a live ghost, not in a separate widget; the player needs spatial context.
- **Per-site-type C++ subclasses for buildable rules.** Rejected: the `Buildable` relationship keeps
  rules data-driven and mod-extensible, consistent with the ECS-everywhere approach (ADR 001).
- **A bare `canPlace` free function as the UI-facing commit (no Action).** Rejected: placement meets
  all three ADR 004 criteria (mutates state, fails with a reason, has draft state), so the commit is a
  `PlaceBuildingAction`/`PlaceConnectorAction` for consistency with every other world mutation. `canPlace`
  survives as the pure kernel *inside* `validate()` — and as the per-frame ghost predicate — but it is
  not what the input/UI code calls to mutate the world.

## Phased Implementation

| Phase | Scope |
|---|---|
| 1 | `Footprint` component + `FixedSize` tag; `Buildable` relationship on sites; `OccupancyGrid` + `buildOccupancyGrid` extracted from the scatter system; `canPlace` (region + connectivity + min-size) with full unit tests. No UI change yet. |
| 2 | Site Window (`modules/window/site_window`) with site info + palette; clicking a site opens it. `BuildMode` singleton; selecting a palette entry enters build mode. |
| 3 | Hover-cell input signal; ghost cursor with valid/invalid rendering (reads `canPlace`); `PlaceBuildingAction`/`PlaceConnectorAction` committing fixed-size single-click placement; retire `systemUpdateConstructionSiteLocations` and the scatter ghosts. |
| 4 | Variable building rectangles: rubber-band drag clamped to a 2×2 minimum and the site bounds, multi-tile region placement, `TooSmall` feedback. |
| 5 | 9-slice rendering for multi-tile buildings (corner/edge/fill from a tilesheet). |
| 6 | Connector click-drag runs (the autotiler from Phase A already fixes variants). |
| A | **Autotiler (independent):** rename `ConstructionSiteNeedsUpdating` → `SiteNeedsRelayout`; the `SiteNeedsRelayout`-driven retile system in `connector.cpp` + the pure `computeConnectorTiling` kernel (ADR 010) with full unit tests. The autotiler coexists with the still-present scatter system (it runs first and lets scatter clear the tag, §6). Depends only on the relayout tag and connector children, so it does not wait on Phases 1–6. |

Each phase is independently shippable. Phase A is listed out of order deliberately: the connector
autotiler shares none of build mode's machinery (`BuildMode`, the Site Window, `canPlace`) and is
implemented first to stop scripted/seeded roads from forming invalid junctions. It leaves the scatter
system in place — interactive building placement keeps working unchanged — and the scatter system is
not retired until Phase 3.

## Non-Goals

- Demolition / bulldoze tooling (warrants its own treatment).
- Terrain, elevation, or non-grid placement.
- Build cost / resource gating (finance integration — ADR 003).
- Rotating buildings; only connectors autotile.
- Rewriting the Lua seed roads to drop their now-redundant explicit variant/rotation — the autotiler
  overrides those at runtime, so the seeds work as-is and need no migration.
- Pathfinding or simulation over the connector network.
