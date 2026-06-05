# 008 — Central Stat Registry via Reflection

**Status:** Proposed

## Context

[ADR 002](/adr/002-stats-and-modifiers.md) established that `Stat` values live as members directly on domain components (e.g. `Launchpad::max_weight`, `Rocket::failure_rate`), keeping each stat co-located with the component that owns it. That decision explicitly accepted one piece of debt:

> Stats are not self-describing in a central registry; discovering which stats exist requires reading component definitions.

In practice that debt has produced two recurring frictions and one latent bug:

1. **No reverse lookup.** A `Modifier` names its target via a `target_stat` string id, but there is no way to go from that id back to the `Stat` instance. The only working direction is `Stat → its modifiers` (used by the tooltip in `stat_widget.cpp`). UI or logic that starts from an effect/modifier cannot reach the stat it affects.

2. **Per-stat manual wiring.** Every owner module hand-writes a system that names each stat field — `statsApplyModifiers(e, &pad.max_weight)` in `site.cpp`, `statsApplyModifiers(rocketE, &rocket.failure_rate)` in `rocket_module.cpp`. Adding a stat means remembering to add a line to the right system.

3. **Latent bug from (2).** `Rocket::cost` is a `Stat` and is registered as a Flecs meta member, but the rocket refresh system only applies modifiers to `failure_rate`. On a live rocket, `cost` never receives its modifiers. This is the failure mode manual wiring invites.

A future goal sharpens the requirement: **Lua mods should be able to define their own components and stats.** A mod-defined component has no C++ type and no pointer-to-member, so any mechanism that depends on compile-time type information cannot describe it.

### Constraint carried forward from ADR 002

Stats **must remain data on their owning component**. They must travel with the component automatically when it is added or removed from an entity (e.g. if a `Rocket` later also becomes a `Payload`, any payload stats must appear and disappear with the `Payload` component, with no separate bookkeeping). This rules out promoting stats back to standalone child entities.

### Key enabling fact

Stat-bearing components **already register their `Stat` members with Flecs reflection**:

```cpp
world.component<Launchpad>()
     .member("max_weight", &Launchpad::max_weight)
     .member("prep_days",  &Launchpad::prep_days);

world.component<Rocket>()
     .member("failure_rate", &Rocket::failure_rate)
     .member("cost",         &Rocket::cost);
```

The ECS therefore already knows, in its meta tables, which component types contain `Stat`-typed members and at what offset. The same is true for components a Lua mod defines at runtime — runtime components populate the identical meta tables. Discovery is largely a matter of *reading metadata that already exists*.

## Options Considered

### Option A — Stats as child entities (revisit ADR 002 Option 1)

Promote each stat to a child entity, named by its id, under its owner. A single global system over `Stat` entities applies modifiers; `target_stat` becomes an entity name lookup.

**Rejected.** It violates the carried-forward constraint: component-intrinsic data would be relegated to external entities, and adding/removing a component (the `Payload` case) would require separate machinery to create and destroy the corresponding stat entities. Data should stay in components.

### Option B — Explicit `stats()` provider

Each stat-bearing component implements `std::vector<Stat*> stats()` and is registered once per type. A generic system applies modifiers; `findStat` walks the provider.

**Rejected.** It duplicates the member list — once in `.member(...)`, once in `stats()` — which is the exact duplication that hid the `cost` bug. More importantly, it depends on a **C++ method** on the struct, which a Lua-defined component cannot provide. Mod support would need a second, parallel registration path.

### Option C — Registration-time accessor capture

Wrap `.member(...)` in a `statMember(...)` helper that also records a typed `(component*) -> Stat*` accessor into a registry. Single source of truth, type-safe, no offset arithmetic.

**Rejected for the mod goal.** The accessor is built from a compile-time pointer-to-member (`&Launchpad::max_weight`). A Lua-defined component has no C++ type and no pointer-to-member, so there is nothing to capture. Mods would again need a separate offset-based (reflective) path — defeating the single-mechanism aim. This option remains attractive for a C++-only world, but loses to Option D once runtime-defined components are a requirement.

## Decision (Option D — Runtime Reflection)

Discover and drive stats by reading Flecs reflection metadata at runtime. Stats stay exactly where ADR 002 put them — as members on their owning components — and domain modules keep registering those members as they already do. No `statMember`, no `stats()` method, no per-stat application system.

A new piece of the stats module owns three responsibilities:

**1. Discovery.** After all modules are imported, walk every struct component in the world. For each member whose registered type is `Stat`, record `(component id, byte offset)`. (Members nested inside sub-structs are reached by recursing the meta graph; this can be deferred until a nested case actually exists.) The result is a registry: *component type → list of stat offsets*.

**2. Automatic application.** For each component type that has at least one stat offset, create one untyped query/system that matches that component and, for every matched entity, computes each `Stat*` as `(char*)component_ptr + offset` and calls the existing `applyModifiers(e, stats)`. This replaces every hand-written per-stat application system. A component added or removed at runtime is matched (or unmatched) automatically by Flecs — no bookkeeping.

**3. Reverse lookup.** Provide `findStat(flecs::entity e, std::string_view id) -> Stat*`: for each registered stat-bearing component that `e` has, resolve its `Stat*`s by offset and return the one whose `id()` matches. This is the missing `modifier → stat` direction.

The existing `applyModifiers(entity, std::vector<Stat*>)` stays public for **transient stats** that do not live on a component instance — e.g. the ad-hoc `cost` stat built in `rocket_prefab_window.cpp`. The reflection layer is additive; it does not replace the low-level utility.

### Why this option

- **Preserves ADR 002's co-location and constraint.** Stats remain component data; nothing moves to external entities; add/remove of a component carries its stats automatically.
- **Single source of truth.** The `.member(...)` registration is the only place a stat is declared. The class of bug where a `Stat` member exists but nobody wired its application (today's `Rocket::cost`) disappears.
- **One data-driven path for C++ *and* Lua components.** Reflection reads the same meta tables for compile-time and runtime components, so a mod that defines a component with a `Stat` member is picked up by application and `findStat` with **zero** additional C++ — the decisive factor given the mod goal. Options B and C are compile-time-bound and would each need a second reflective mechanism for mods.
- **Type safety is not abandoned.** Discovery reads *registered* metadata and validates `member.type == id<Stat>()`; it does not guess offsets.

### Tradeoffs accepted

- **Lower-level code in the stats module.** Offset arithmetic and untyped queries live in one place, written and tested once, rather than typed `.each` loops in each domain. This is the same machinery mod support would require regardless.
- **Pointer stability.** A `Stat*` resolved by offset points into component storage and is only valid while that storage is not relocated. Application happens inside the system iteration; `findStat` returns a transient pointer to be used immediately and never stored. This must be documented at the API.
- **Discipline still required at the `.member` call.** A `Stat` member that is never registered via `.member(...)` is invisible to discovery. This is the same convention the codebase already follows, and a forgotten registration fails the same way it does today (no modifiers applied), now localized to one line.

### Out of scope

Exposing a runtime component-definition API to Lua (so mods can actually declare components with `Stat` members) is a separate capability and is **not** part of this decision. This ADR ensures that once such components exist, modifier application and lookup require no further work. The `Stat` type is already Lua-registered.

## Consequences

- **Reverse lookup exists** — `findStat(entity, id)` resolves a modifier's `target_stat` back to its `Stat`, enabling effect-centric UI and logic.
- **No per-stat wiring** — the per-domain application systems in `site.cpp` and `rocket_module.cpp` are removed; adding a stat is just adding a `Stat` member and its `.member(...)` line.
- **Bug class eliminated** — every registered `Stat` member is applied; `Rocket::cost` is fixed as a side effect.
- **Mod-ready** — Lua-defined components with stats are first-class through the same path, with no C++ changes.
- **Centralized cost** — application now iterates all stat-bearing components in one place. The recompute concern from ADR 002 (reset-and-reaccumulate each update) is unchanged and still a future caching candidate.
- **One reflective subsystem to maintain** — the discovery/offset logic is concentrated in the stats module and must be covered by tests (a component with stats is discovered and applied; `findStat` matches by id; an unregistered member is absent).
