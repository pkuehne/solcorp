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

### Option 3 — Explicit state components + Actions for all transitions (chosen)

Introduce explicit state enums on `RocketState` and `LaunchPlanState`. All transitions — user-triggered and automated — use the Action pattern from ADR 004. Automated completions attach `EffortRequired` / `DurationRequired` progress components and a pending-transition intent component (`PendingStateTransition` or `PendingMove`); the progress system decrements them and fires the completion Action when both reach zero.

**Pros:** state is explicit and visible; Actions are the single path for all transitions — the same `validate()` / `execute()` contract guards both button presses and timer expirations; testable independently of UI; no parallel completion utility to maintain.  
**Cons:** existing ad-hoc mutations must be replaced with Action calls; Lua bindings must be tightened.

## Decision

Introduce explicit state components for rockets and launch plans. All transitions — whether initiated by a player button press or by a timer expiring — use the Action pattern from ADR 004. `validate()` checks domain preconditions and returns a `ValidationResult`. `execute()` mutates state and, where a timed step follows, attaches progress components and a pending-transition intent component. The progress system calls the completion Action; it does not mutate state directly.

```
User action  ──────────────────────────────────────────────┐
                                                            ▼
Progress system (each game day)                    Action::validate()  — domain preconditions
 └─ decrement EffortRequired / DurationRequired    Action::execute()   — mutate state, attach progress
 └─ when both zero: construct + execute                     │
      completion Action                                     ├─ guards met:    advance state, remove progress
                                                            └─ guards unmet:  attach TransitionBlocked{reason}
                                                                              retry each tick until resolved
```

Physical location continues to use Flecs parent/child relationships and is not part of state. Moves that have effort or duration requirements attach `PendingMove { destination }` alongside the progress components; no state change occurs, only a parent change.

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
    // target is carried by PendingStateTransition when a timed transition is in progress
};

enum class LaunchPlanStateId : uint8_t {
    Scheduled,  // first ECS state — Draft exists only as UI state in LaunchWindow
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

### Progress and Intent Components

`EffortRequired` and `DurationRequired` are reusable components that express orthogonal facts about a pending transition. A worker system that decrements `EffortRequired.remaining` does not need to know what type of transition is pending.

```cpp
struct EffortRequired {
    int total     = 0;
    int remaining = 0;  // decremented by worker/facility systems
};

struct DurationRequired {
    int total     = 0;
    int remaining = 0;  // decremented each game day
};
```

An intent component is attached alongside the progress components to tell the completion system which Action to fire:

```cpp
struct PendingStateTransition {
    RocketStateId target;
};

struct PendingMove {
    flecs::entity destination;
    // no target state — only the parent changes
};
```

When `EffortRequired.remaining` and `DurationRequired.remaining` (whichever are present) both reach zero, the progress system constructs and executes the appropriate completion Action, then removes the progress and intent components.

`TransitionBlocked` is set by a completion Action whose `validate()` fails at execution time — world state changed after the transition started (funds spent elsewhere, launchpad reassigned, building demolished). It is an alarm, not a normal "still working" status.

```cpp
struct TransitionBlocked {
    std::string reason;
};
```

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
rocket.get_mut<RocketState>().current = RocketStateId::IntegratingPayload;
rocket.set<PendingStateTransition>({RocketStateId::IntegrationComplete});
rocket.set<DurationRequired>({.total = 5, .remaining = 5});
rocket.child_of(vab_entity);
```

Progress system (automated):

```cpp
world.system<DurationRequired>("AdvanceDuration")
    .kind(UpdatePhase)
    .each([](flecs::entity e, DurationRequired& d) {
        if (d.remaining > 0) --d.remaining;
        if (d.remaining == 0 && !e.has<EffortRequired>())
            // construct and execute the appropriate completion Action
            fire_completion_action(e);
    });
```

### Example Workflows

| Phase | How triggered | Progress type |
|---|---|---|
| Build | Action | `EffortRequired` |
| Payload integration | Action | `DurationRequired` |
| Rollout | Action | `DurationRequired` |
| Construction complete | completion Action (fired by progress system) | — |
| Launch plan readiness | completion Action (fired by progress system) | guard: rocket parent is launchpad |

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
- **Single transition path** — Actions handle both user-triggered and automated completions; the progress system fires the completion Action rather than mutating state directly. `TransitionBlocked` catches cases where world state changed after a transition started.
- **Better UI feedback** — `ValidationResult` explains why an action was rejected before it started; `TransitionBlocked` is an alarm for when world state changed under an in-progress transition, with a specific reason the player can act on.
- **Testable** — Actions take a `flecs::world&`; tests construct a world, set up state, call `validate()` / `execute()`, assert on components.
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
2. Add `EffortRequired` / `DurationRequired` / `PendingStateTransition` / `PendingMove` components
3. Add progress systems (decrement counters, fire completion Actions)
4. Add `TransitionBlocked` handling and retry
5. Implement Actions for each transition (user-triggered and completion)
6. Build Rocket List UI
7. Build Launch Plan status UI
8. Add unit tests (valid/invalid Actions, blocked completion, retry after blocker removed, effort and duration progression, cancellation paths)
