# 006 — Rocket and Launch Plan Workflow

**Status:** Accepted

## Context

The 0.1 MVP rocket launch workflow is functional but limited.

Current behaviour:

- Rockets are built over time
- Rockets can be manually moved between buildings
- Launches are scheduled using launch plans
- Validation exists before scheduling launches
- Cooldowns and prep delays are modelled with ad-hoc logic

Problems with the current approach:

1. Rocket lifecycle state is implicit and spread across multiple systems
2. Launch plan progression is static rather than operational
3. Timed work (integration, rollout, prep) is inconsistent
4. Blocked automated workflows are hard to represent or retry
5. UI does not explain why something cannot proceed

We need a scalable workflow model for 0.2 that supports rocket construction, storage logistics, payload integration, rollout to launchpad, launch readiness, cancellation and delays, and future expansion.

## Options Considered

### Option 1 — Extend the existing Action Pattern alone

Keep the `IAction` / `validate()` / `execute()` pattern from ADR 004 and add more action structs for each workflow step.

**Pros:** no new concepts; consistent with existing code.  
**Cons:** actions handle user-triggered steps well but cannot model automated completions — when a construction timer expires there is no user action to trigger the state advance. Rocket state remains implicit; nothing prevents a system from leaving it in an impossible combination.

### Option 2 — Ad-hoc state flags per system

Add boolean/enum fields to existing components (`is_integrating`, `is_rolling_out`, etc.) and handle transitions inline in each system.

**Pros:** minimal new code.  
**Cons:** state is scattered; systems must defensively check each other's flags; no single authoritative definition of what states are legal in combination.

### Option 3 — Explicit state components + Actions + completion utility (chosen)

Introduce explicit state enums on `RocketState` and `LaunchPlanState`. User-triggered transitions remain Actions (full domain validation + state mutation). Automated completions are handled by a `complete_transition` free function called by the progress system.

**Pros:** state is explicit and visible; Actions retain full domain awareness; automated completions are modelled cleanly without duplicating Action machinery; testable independently of UI.  
**Cons:** existing ad-hoc mutations must be replaced with Action calls or completion utility calls; Lua bindings must be tightened.

## Decision

Introduce explicit state components for rockets and launch plans. The workflow has two distinct transition paths, each handled differently:

**User-triggered transitions** use the Action pattern from ADR 004. `validate()` checks domain preconditions (funds available, target location free, rocket in correct state, etc.) and returns a `ValidationResult`. `execute()` mutates state and attaches progress components. Actions are the authoritative layer for anything requiring domain knowledge.

**Automated completions** use a `complete_transition` free function called by the progress system when `EffortRequired` or `DurationRequired` is satisfied. This function advances state to the target and removes progress components. If a completion guard is unmet (e.g. rocket not yet on pad), it attaches a `TransitionBlocked` component and the system retries each tick.

```
User action
 └─ Action::validate()   — domain preconditions (funds, location, state validity)
 └─ Action::execute()    — set state + target, attach progress components

Progress system (each game day)
 └─ advance EffortRequired / DurationRequired
 └─ when complete: complete_transition(entity)
      ├─ guards met:    advance state to target, remove progress + blocker
      └─ guards unmet:  attach TransitionBlocked{reason}, retry next tick
```

Physical location continues to use Flecs parent/child relationships and is not part of state.

### State Components

```cpp
enum class RocketStateId : uint8_t {
    UnderConstruction,
    Stored,
    Assigned,
    IntegratingPayload,
    IntegrationComplete,
    RollingOut,
    OnPad,
    Launched,
    Unavailable
};

struct RocketState {
    RocketStateId current;
    RocketStateId target;  // set while a timed transition is in progress
};

enum class LaunchPlanStateId : uint8_t {
    Draft,
    Scheduled,
    WaitingForRocket,
    Integrating,
    WaitingForRollout,
    ReadyToLaunch,
    Launched,
    Cancelled,
    Failed
};

struct LaunchPlanState {
    LaunchPlanStateId current;
    LaunchPlanStateId target;
};
```

### Progress Components

```cpp
struct EffortRequired {
    int total   = 0;
    int current = 0;
};

struct DurationRequired {
    int total   = 0;
    int current = 0;
};
```

### Completion Utility

```cpp
// Called by progress systems — not by UI or Actions
void complete_transition(flecs::entity entity);
```

On success: advances `current` to `target`, removes progress components, removes any `TransitionBlocked`.  
On failure: attaches `TransitionBlocked{reason}` and raises a player notification, retried each tick until resolved or cancelled.

```cpp
struct TransitionBlocked {
    std::string reason;
};
```

`TransitionBlocked` represents a situation the player did not anticipate: the Action's `validate()` passed, the transition started, but world state changed before completion — funds were spent elsewhere, a launchpad was assigned to another rocket, a building was demolished. The player needs to know and may need to intervene. It is not a normal "still working" status; it is an alarm.

### Example: Payload Integration

Action (user-triggered):

```cpp
// StartIntegrationAction::validate()
if (rocket.get<RocketState>()->current != RocketStateId::Stored)
    return ValidationResult::Fail("Rocket is not in storage");
if (!vab_has_free_slot(world))
    return ValidationResult::Fail("No VAB slot available");
// ... other domain checks

// StartIntegrationAction::execute()
rocket.set<RocketState>({RocketStateId::IntegratingPayload, RocketStateId::IntegrationComplete});
rocket.set<DurationRequired>({5, 0});
rocket.child_of(vab_entity);
```

Progress system (automated):

```cpp
world.system<DurationRequired>("AdvanceDuration")
    .kind(UpdatePhase)
    .each([](flecs::entity e, DurationRequired& d) {
        d.current++;
        if (d.current >= d.total)
            complete_transition(e);
    });
```

### Example Workflows

| Phase | How triggered | Progress type |
|---|---|---|
| Build | Action | `EffortRequired` |
| Payload integration | Action | `DurationRequired` |
| Rollout | Action | `DurationRequired` |
| Construction complete | `complete_transition` | — |
| Launch plan readiness | `complete_transition` | guard: rocket parent is launchpad |

### Lua Integration

Lua may read any component. Lua must not directly mutate `RocketState`, `LaunchPlanState`, `TransitionBlocked`, progress components, or operational parent relationships.

Lua calls exposed APIs that map to Action `execute()` calls:

```lua
rocket:start_integration()
rocket:begin_rollout()
launchplan:schedule(...)
```

### UI Requirements

**Rocket list window** displays per rocket: name, type, current state, target state (if transitioning), location, progress, blocked reason.

```
Atlas-3  IntegratingPayload → IntegrationComplete  VAB-1    3/5 days
Nova-2   WaitingForRollout                          BLOCKED: No pad available
```

**Launch plan window** displays per plan: mission name, assigned rocket, pad, state, launch day, blocked reason.

## Consequences

- **Explicit state** — `RocketState` and `LaunchPlanState` make the lifecycle visible and queryable; impossible combinations are caught by Action validation.
- **Actions retain domain awareness** — funds, location availability, and other preconditions stay where they belong, in `validate()`.
- **Automated completions are first-class** — `complete_transition` + `TransitionBlocked` cleanly models the cases no Action can handle.
- **Better UI feedback** — `ValidationResult` explains why an action was rejected before it started; `TransitionBlocked` is an alarm for when world state changed under an in-progress transition, with a specific reason the player can act on.
- **Testable** — Actions and `complete_transition` take a `flecs::world&` or `flecs::entity`; tests construct a world, set up state, call the function, assert on components.
- **Migration cost** — ad-hoc state mutations must be replaced; Lua direct writes must be replaced with API calls.

## Non-Goals for 0.2

- Weather system
- Crew assignment
- Fueling simulation
- Multiple simultaneous blockers per entity
- Generic reusable FSM framework
- Pad-specific state

## Implementation Order

1. Add `RocketState` / `LaunchPlanState` components
2. Implement `complete_transition` utility
3. Add progress systems for effort and duration
4. Add `TransitionBlocked` handling and retry
5. Implement Actions for each user-triggered transition
6. Build Rocket List UI
7. Build Launch Plan status UI
8. Add unit tests (valid/invalid Actions, blocked completion, retry after blocker removed, effort and duration progression, cancellation paths)
