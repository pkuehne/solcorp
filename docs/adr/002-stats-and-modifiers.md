# 002 — Stats and Modifiers

**Status:** Accepted

## Context

Game entities (launchpads, sites, facilities) need configurable numeric stats — things like payload capacity or preparation time. These stats must be:

- **Moddable** — mods can add new stats and apply modifiers without touching C++
- **Hierarchy-aware** — an effect placed on a site should influence all buildings on it
- **Tooltip-friendly** — the UI should be able to show a breakdown of what is affecting a value

Three options were considered.

## Options Considered

### Option 1 — Pure ECS Stat Entities

All stats and modifiers are Flecs entities. A stat is a child entity of its owning entity with a `Stat` component. Modifiers are child entities of `Effect` entities, linked to the owning entity via a `HasEffect` relationship.

```
Entity ("Cape Canaveral")
  └─ HasEffect → Entity ("Forklift Trucks")
                   └─ Entity ("Loading Time Modifier")
                        Component (Modifier) { additive: -1, target: "Loading Time" }
Entity ("Launchpad A")  [child of Cape Canaveral]
  └─ Entity ("Loading Time")
       Component (Stat) { base, final }
```

**Pros:** fully moddable, effects can carry multiple modifiers, hierarchy position controls scope.  
**Cons:** stat values are not co-located with their owning component; many extra entities.

### Option 2 — Stats Module with Accessor Functions

Stats are stored in a `StatBlock` component (`std::map<string, Stat>`) on a dedicated entity linked to a prefab via a custom `UsesStats` relationship. Global stat definitions live under a `Stats` root entity. Access is via free functions that walk the hierarchy:

```cpp
int loading_time = stat_get_value("loading_time", entity);
stat_draw_value("loading_time", entity);  // renders tooltip inline
```

**Pros:** access pattern is simple and uniform; stat definitions are centralised.  
**Cons:** `std::map` lookup per stat per frame; hierarchy traversal on every read; stat definitions are decoupled from the components that use them, making discoverability harder.

## Decision (Option 3 — Implemented)

Stats are stored directly as `Stat` members on domain components. Each `Stat` carries its own id, display name, description, base value, and accumulated modifiers. This keeps the stat co-located with the component that owns it — a `Launchpad` holds its own `prep_days` stat rather than delegating to a separate entity.

Effects are standalone entities tagged with the `Effect` component and attached to a target entity via the `HasEffect` relationship. An effect's child entities carry `Modifier` components that name a target stat id, an additive offset, and a multiplicative factor.

Each module that owns stats runs a system in `UpdatePhase` that calls `applyModifiers`. That utility walks the entity's ancestor chain, collects every `HasEffect` relationship, then iterates each effect's children for matching `Modifier` components, accumulating additive and multiplicative values onto the relevant `Stat`.

The final value is computed as:

```
final = (base + Σ additive) × Π multiplicative
```

The `displayStatWithTooltip` helper renders the stat value with a hover tooltip that lists each contributing effect, its modifier value, and the base and final values — colour-coded by whether higher is better.

## Structure

```cpp
// Stat — embedded in domain components (e.g. Launchpad, RocketStats)
Stat prep_days{"prep-days", "Prep Days", "Days to prepare a launch", 10.0};

// Effect — a named entity with the Effect tag, attached via HasEffect
world.entity("Cape Canaveral")
     .add<HasEffect>(world.entity("Better Concrete"));

// Modifier — child of an effect entity
world.entity("loading_time_mod")
     .set<Modifier>({"prep-days", -1.0, 1.0})
     .child_of(effect);

// Applying modifiers (called each UpdatePhase)
std::vector<Stat*> stats = {&launchpad.prep_days, &launchpad.max_weight};
applyModifiers(entity, stats);
```

Effect scope is determined by position in the ECS hierarchy: an effect on a site entity applies to all buildings and facilities beneath it; an effect on a single building applies only there.

## Consequences

- **Co-location** — stats live with the components that use them; no indirection to find a stat's value.
- **Moddable** — effects and modifiers are created entirely from Lua; new stats can be added by adding a `Stat` member to a component.
- **Tooltip breakdown** — `displayStatWithTooltip` provides the per-effect breakdown without extra bookkeeping.
- **Hierarchy scope** — effect placement in the entity tree naturally scopes its influence, matching the mental model of "a site-wide upgrade affects everything on the site."
- **Recompute cost** — `applyModifiers` resets and re-accumulates all modifiers each update. This is acceptable at current entity counts but would need caching if the number of affected entities grows significantly.
- **Stat–component coupling** — stats are not self-describing in a central registry; discovering which stats exist requires reading component definitions.

## Auto-discovery of stat updaters (later addition)

Initially each module manually registered a per-component `UpdatePhase` system to call `statsApplyModifiers`.  This created a third registration obligation alongside the Flecs reflection call (`.member()`) and the Lua binding (`register_component_lua`).

The Flecs meta addon stores member type information (entity ID + byte offset) for every struct registered with `.member()`. Since `Stat` is itself a registered component, any field declared as `Stat` in another component will have its `type` field set to `ecs_id(Stat)`. `StatsModule` exploits this in `discover_stat_update_systems()`:

1. Query all entities that carry `EcsComponent` (every registered component type).
2. For each, retrieve the `EcsStruct` metadata; skip components with no struct metadata.
3. Scan the `ecs_member_t` list for entries whose `type == ecs_id(Stat)`.
4. If any are found, dynamically register an `UpdatePhase` system that uses `ecs_field_w_size` to access the raw component array and calls `statsApplyModifiers` at each discovered offset.

`discover_stat_update_systems` is called automatically from a `PostStartPhase + immediate` system in `StatsModule`, which runs after all module constructors have completed and before the first `UpdatePhase`.

**Consequence:** adding a new `Stat` field to any component now requires only:

1. Declare the field in the struct header.
2. Add `.member("field_name", &T::field_name)` to the existing `world.component<T>()` call.

No separate update system is needed.  The Flecs `.member()` call that was already required for the REST inspector and Lua serialisation now also drives the stats update wiring.
