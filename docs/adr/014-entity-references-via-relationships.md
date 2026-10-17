# 014 — Entity References via Relationships

**Status:** Accepted

## Context

Game-state entities often need to point at other entities: a launch plan at its target orbit, a
sprite at its texture, a connector layer at its tileset. The project already models most of these
links as Flecs relationships — `HasEffect`, `TeamMember`, `LaunchingOn`, the exclusive state pairs
(`RocketCurrentState`, `CurrentWeather`), and the connector layer slots of
[ADR 010](/adr/010-connector-tile-system.md) — and ADR 010 states the rule in passing ("use Flecs
relationships, not raw IDs in component fields").

The rule has never been decided on its own, so it is applied unevenly. A few components still hold a
`flecs::entity` member:

| Component | Field | Points at |
|---|---|---|
| `Sprite` | `texture` | a texture entity (#232) |
| `LaunchPlan` | `target_orbit` | an orbit |
| `Simulation` | `speed` | the simulation timer |

and [ADR 009](/adr/009-build-mode-and-site-window.md) specifies one more (`BuildMode::prefab`).

A stored entity handle has three problems that a relationship does not:

- **It dangles.** Deleting the target leaves the field holding a dead entity that every reader has
  to check for.
  Flecs removes a relationship pair automatically when its target is deleted, and `OnDelete`
  policies can delete or clean up the source instead.
- **It cannot be queried.** "Every launch plan targeting low Earth orbit" is a single query term for a
  pair (`.with<TargetOrbit>(leo)`); for a field it is a scan over every plan.
- **It is invisible to tooling.** Pairs show up in the Flecs explorer (REST API) as links between
  entities; a `flecs::entity` field also carries a world pointer that does not serialise.

## Decision

**Components on game-state entities do not store entity references. A link from one entity to
another is a relationship.**

- **One target** (an orbit, a texture, a tileset): a relationship tag marked `flecs::Exclusive`, so
  adding a new target replaces the old one. Read with `e.target<R>()`.
- **Many targets** (effects, team memberships): a plain relationship tag.
- **A link that carries data**: a pair with data, as `CurrentWeather` already does
  (`site.set<CurrentWeather>(pattern, {...})`).
- **Ownership**: `ChildOf`, as today.

### Scope

The rule covers components that make up the **game state**: data on sites, buildings, rockets, plans,
tiles and the singletons that describe the world. It does **not** cover values that live outside the
world or only for a moment:

- **Actions** ([ADR 004](/adr/004-action-pattern.md)) — short-lived C++ objects that hold the
  entities they act on and are thrown away after `execute()`.
- **Function parameters, locals and returned values** — e.g. the `WeatherTransitionData` list a
  system builds and consumes in one call.
- **UI window state** — the entity a window is showing or filtering by. This is view state, not game
  state: it is not part of a save and is cheap to re-select. Windows must still check `is_alive()`
  before using it.

### Migration

The rule applies to new code immediately. Existing fields are migrated one at a time, each as its own
change: `Sprite::texture` (#232), `LaunchPlan::target_orbit`, and `BuildMode::prefab` when ADR 009
Phase 2 is built. `Simulation::speed` holds a Flecs timer, which is engine plumbing rather than a game
link; it is left as is unless that changes.

## Consequences

- Deleting an entity cleans up every link to it; no stale handles in game state.
- Links are queryable and visible in the Flecs explorer.
- Components stay plain data, which keeps them serialisable for a future save system.
- Each kind of link needs its own relationship type, and single-target ones need the `Exclusive`
  trait registered. Getting the target is `e.target<R>()` instead of a field read.
- Lua has no relationship API, so each link exposed to mods needs a helper function (as
  `create_connector` does for tileset layers).

## Alternatives Considered

- **Raw `ecs_entity_t` ids in components**: serialisable, but still dangles and still cannot be
  queried.
- **Keep `flecs::entity` fields where convenient**: the status quo; it is how the inconsistencies above
  arose.
- **Apply the rule to UI window state as well**: rejected for now — a lot of churn for state that is
  short-lived and not saved. Revisit if window state becomes part of a save.

## Related

- [ADR 001](/adr/001-ecs-with-flecs.md) — ECS with Flecs (`ChildOf` and `IsA` instead of id fields).
- [ADR 009](/adr/009-build-mode-and-site-window.md) — `BuildMode::prefab` follows this rule.
- [ADR 010](/adr/010-connector-tile-system.md) — connector layer slots, the first stated use of the
  rule.
- [ADR 013](/adr/013-tilesets.md) — tileset links (`TilesetTexture`, connector layers).
- [Flecs Relationships](/architecture/flecs-relationships.md) — how relationships are used in the
  codebase today.
