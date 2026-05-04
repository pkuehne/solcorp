# 006 — Rocket and Launch Plan FSM

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
4. Blocked workflows are hard to represent
5. UI does not explain why something cannot proceed
6. Lua exposing writable components risks invalid state mutation
7. The existing action system (see [ADR 004](004-action-pattern.md)) duplicates validation and execution logic across the rocket/launch domain

We need a scalable workflow model for 0.2 that supports rocket construction, storage logistics, payload integration, rollout to launchpad, launch readiness, cancellation and delays, manual and automatic movement, and future expansion.

## Options Considered

### Option 1 — Extend the existing Action Pattern

Keep the `IAction` / `validate()` / `execute()` pattern from ADR 004 and add more action structs for each new workflow step (integrate payload, rollout, etc.).

**Pros:** no new concepts; consistent with existing code.  
**Cons:** each action encodes its own preconditions independently, so there is no single place that enforces legal state transitions. The rocket can reach impossible combinations of component state because nothing owns the full lifecycle. UI feedback remains per-action rather than per-entity.

### Option 2 — Ad-hoc state flags per system

Add boolean/enum fields to existing components (`is_integrating`, `is_rolling_out`, etc.) and handle transitions inline in the systems that own each phase.

**Pros:** minimal new code.  
**Cons:** state is scattered; systems must defensively check each other's flags; no single authoritative definition of what states are legal in combination.

### Option 3 — Explicit FSMs owning all lifecycle transitions (chosen)

Introduce `RocketFSM` and `LaunchPlanFSM` as the sole owners of state mutation. Progress components (`EffortRequired`, `DurationRequired`) drive timing without knowing about state. A `TransitionBlocked` tag explains stalls to the UI.

**Pros:** single authoritative transition graph; unified validation; UI feedback is structural (read the blocker component) not ad-hoc; testable state logic independent of rendering.  
**Cons:** requires refactoring existing actions into FSM wrappers; more explicit upfront state definitions needed; Lua bindings must be tightened.

## Decision

Introduce two explicit finite state machines:

- `RocketFSM` — owns the rocket entity lifecycle
- `LaunchPlanFSM` — owns the mission workflow

These are the sole owners of all operational state transitions. Timed and effort-based work is represented by progress components (`EffortRequired`, `DurationRequired`). When requirements are complete, the FSM attempts to complete the active transition. If completion preconditions fail, the entity receives a `TransitionBlocked` component and retries automatically on future ticks.

Physical location continues to use existing Flecs parent/child relationships and is not part of FSM state.

### Core Principles

**Operational state is separate from physical location.** Operational state is held in FSM state components. Physical location is determined by parent entity:

```
Storage Building
└── Rocket A

VAB
└── Rocket B

Launchpad A
└── Rocket C
```

**FSM owns state mutation.** Rocket or launch plan state must not be directly edited by UI or Lua. All changes go through FSM functions:

```cpp
RocketFSM::try_start_transition(world, rocket, transition);
RocketFSM::complete_transition(world, rocket);
RocketFSM::cancel_transition(world, rocket);
RocketFSM::fail_transition(world, rocket);

LaunchPlanFSM::try_start_transition(world, plan, transition);
LaunchPlanFSM::complete_transition(world, plan);
```

**Progress systems track work.** Cheap ECS systems iterate entities with `EffortRequired` or `DurationRequired`, update progress, and invoke FSM completion when requirements are satisfied. They do not directly mutate final state.

**Blocked transitions use an exception component.** If a transition is ready to complete but cannot due to unmet guards, the entity receives:

```cpp
struct TransitionBlocked {
    std::string reason;
};
```

Absence means normal flow. Presence means stalled progression. The reason string is shown directly in the UI.

### Component Model

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
    RocketStateId target;
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

struct EffortRequired {
    int total = 0;
    int current  = 0;
};

struct DurationRequired {
    int total = 0;
    int current  = 0;
};

struct TransitionBlocked {
    std::string reason;
};
```

### FSM Responsibilities

**RocketFSM** owns the rocket entity lifecycle:

States: 

- `UnderConstruction`
- `Stored`
- `Assigned`
- `IntegratingPayload`
- `IntegrationComplete`
- `RollingOut`
- `OnPad`
- `Launched`
- `Unavailable`

Controls: parent movement between buildings, progress component setup/removal, launch readiness state changes.

**LaunchPlanFSM** owns the mission workflow:

States: 
- `Draft`
- `Scheduled`
- `WaitingForRocket`
- `Integrating`
- `WaitingForRollout`
- `ReadyToLaunch`
- `Launched`
- `Cancelled`
- `Failed`

Controls: payload assignment, pad reservation, schedule state, mission readiness. Uses `RocketFSM` for rocket-side actions.

### Transition Lifecycle

Starting a transition (example: payload integration):

```cpp
RocketFSM::try_start_transition(
    world, rocket, RocketTransition::StartPayloadIntegration);
// Sets current = IntegratingPayload, target = IntegrationComplete
// Attaches DurationRequired{5, 0}
// Moves rocket to VAB if required
```

Progress system (runs each game day):

```cpp
duration.elapsed_days++;
effort.current += assigned_team_output;
```

Attempting completion (called by progress system when requirements are met):

```cpp
RocketFSM::complete_transition(world, rocket);
// Success: state advances to target, progress and blocker components removed
// Failure: attaches TransitionBlocked{"Rocket is not at launchpad"}
//          system retries next tick
```

### Example Workflows

| Phase | Transition | Progress type |
|---|---|---|
| Build | `UnderConstruction → Stored` | `EffortRequired` |
| Integration | `Stored → IntegratingPayload → IntegrationComplete` | `DurationRequired` |
| Rollout | `IntegrationComplete → RollingOut → OnPad` | time or effort |
| Launch plan readiness | `WaitingForRollout → ReadyToLaunch` | guard: rocket parent is launchpad |

### Migration from Existing Actions

The `validate()` / `execute()` shape from ADR 004 maps directly:

```
validate()  →  try_start_transition()
execute()   →  complete_transition()
```

Existing action structs (`MoveRocketAction`, `ScheduleLaunchAction`) become thin wrappers around FSM APIs. The `IAction` interface remains valid for UI-driven operations that don't involve FSM-owned state.

### Lua Integration

Lua may read any component. Lua must not directly mutate:

- `RocketState` or `LaunchPlanState`
- `TransitionBlocked`
- progress components (`EffortRequired`, `DurationRequired`)
- operational parent relationships

Lua calls exposed APIs that route to FSM functions:

```lua
rocket:start_transition(...)
rocket:move_to_storage(...)
launchplan:schedule(...)
```

### UI Requirements

**Rocket list window** displays per rocket: name, type, current state, target state (if transitioning), parent building/location, progress, blocked reason.

Example:

```
Atlas-3  IntegratingPayload → IntegrationComplete  VAB-1       3/5 days
Nova-2   WaitingForRollout                          BLOCKED: Rocket not on pad
```

**Launch plan window** displays per plan: mission name, assigned rocket, pad, state, launch day, blocked reason.

## Consequences

- **Clear operational model** — a single transition graph defines what state combinations are legal.
- **Unified validation** — preconditions live in FSM guards, not scattered across actions.
- **Better UI feedback** — `TransitionBlocked.reason` is always a specific, displayable message.
- **Natural support for delays and failures** — progress components and retry logic are structural.
- **ECS-aligned** — progress and blocker state are components; systems iterate them cheaply.
- **Testable** — state transitions can be tested without UI or rendering harness.
- **Refactor cost** — existing direct mutations and action structs must be updated.
- **Tighter Lua bindings** — direct component writes that currently work must be replaced with API calls.

## Non-Goals for 0.2

- Weather system
- Crew assignment
- Fueling simulation
- Multiple simultaneous blockers per entity
- Generic reusable FSM framework
- Pad-specific FSM

## Implementation Order

1. Add `RocketState` / `LaunchPlanState` components
2. Implement `RocketFSM`
3. Implement `LaunchPlanFSM`
4. Add progress systems for effort/time
5. Add `TransitionBlocked` handling
6. Refactor current actions to FSM wrappers
7. Build Rocket List UI
8. Build Launch Plan status UI
9. Add unit tests (valid/invalid/blocked transitions, retry after blocker removed, time and effort completion, parent/location correctness, cancellation paths)
