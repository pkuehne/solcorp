# 007 — Launch Plan Timeline and Automation

**Status:** Accepted

## Context

ADR 006 defines the rocket and launch plan state machine and establishes that the Action pattern drives all transitions. What it does not define is how the schedule is computed, where stage durations come from, how auto-progression interacts with manual control, or how the stage sequence can be extended by mods.

At the time of writing:
- `LaunchPlan` stores only `launch_date` and `target_orbit`
- `Launchpad` has a `prep_days` Stat (on-pad time before launch)
- `RocketMoveAction` hardcodes its duration — no stat governs rollout time
- Pad conflict validation only runs at plan-creation time

## Decisions

### 1. Rollout duration is a Stat on `Rocket`

The move from storage to launchpad ("rollout" — the industry term for e.g. the crawler-transporter at KSC) takes different amounts of time depending on vehicle design. It belongs on the rocket so modifiers can apply.

```cpp
struct Rocket {
  Stat rollout_days = Stat({
    .id = "rollout-days",
    .display = "Rollout Duration",
    .description = "Days to move the rocket from storage to the launchpad",
    .base = 3,
    .higher_is_better = false
  });
};
```

If an intermediate integration facility (VAB) is added later, rollout remains the word for the final leg to the pad; the earlier leg gets its own name.

### 2. Milestone dates are computed at plan creation and stored in `LaunchPlan`

```
arrival_date = launch_date - prep_days       // first day rocket is on the pad
rollout_date = arrival_date - rollout_days   // last day rollout can start on schedule
```

Stored on `LaunchPlan` so the UI and progression system read them directly without querying stats:

```cpp
struct LaunchPlan {
  uint32_t launch_date  = 0;
  uint32_t arrival_date = 0;
  uint32_t rollout_date = 0;
  flecs::entity target_orbit = flecs::entity::null();
};
```

`EditLaunchAction` recomputes both dates from the updated `launch_date` and current stats.

### 3. Pad occupancy window is `[arrival_date, launch_date]`

Conflict validation checks this window, not just the launch day. Two plans on the same pad conflict if their windows overlap. The existing check (`[launch_date - prep_days, launch_date)`) is equivalent; this ADR formalises it using the named field.

### 4. Auto-progression and manual triggers use the same Actions

The progress system fires completion Actions when `DurationRequired` reaches zero. For date-gated steps a daily system fires the begin Action when `today >= milestone_date`. Manual UI buttons call the same begin Action directly, meaning the player can start rollout early if the rocket and pad are ready — `validate()` requires state preconditions, not the date.

```
Auto:   daily system  ──►  InitiateRolloutAction::validate / execute
Manual: player button ──►  InitiateRolloutAction::validate / execute
```

The daily system never mutates state directly.

### 5. Stage types are ECS entities; systems are the dispatch

Stage types follow the same pattern as rocket states — they are world entities, not enum values:

```
Stages::LaunchPlan::Rollout
Stages::LaunchPlan::OnPad
Stages::LaunchPlan::Launched
...
```

`LaunchPlanCurrentStage` is an exclusive relationship to the current stage entity. Each stage's logic lives in a Flecs system filtered to that entity — the filter *is* the dispatch, no registry required:

```cpp
auto rollout_stage = world.entity("Stages::LaunchPlan::Rollout");

world.system("AutoProgressRollout")
    .with<LaunchPlanCurrentStage>(rollout_stage)
    .without<DurationRequired>()
    .each([](flecs::entity plan) {
        if (today(plan.world()) < plan.get<LaunchPlan>()->rollout_date) return;
        InitiateRolloutAction action{plan};
        auto result = action.validate(plan.world());
        if (result) { action.execute(plan.world()); }
        else { plan.set<TransitionBlocked>({result.message}); }
    });
```

A mod adds a new stage by creating a new entity and registering its own system with `.with<LaunchPlanCurrentStage>(mod_stage_entity)`. No existing code changes.

`LaunchProfile` holds an ordered sequence of stage entities. When a stage completes, the system finds the current entry in the vector and advances to the next:

```cpp
struct LaunchProfile {
    std::vector<flecs::entity> stages;
};
```

Without a profile the default sequence is the same vector, hardcoded in the module constructor. `LaunchProfile` makes the sequence data-driven without changing the mechanism.

**Do not implement `LaunchProfile` yet.** The fixed-stage sequence is correct until there is a concrete reason to diverge. The entity-and-system pattern is already compatible with profiles when the time comes.

**Manual trigger validation for mod stages:** built-in stage buttons construct the corresponding IAction and call `validate()` for immediate UI feedback. Mod stage buttons read `TransitionBlocked` set by the system on its previous attempt — a retry-and-report model. A future hook on the stage entity could let mods provide a validate callback for richer pre-click feedback, but that is out of scope.

## Actions Design

### Stage action pairs

| Stage | Begin action | Completion trigger | Completion action |
|---|---|---|---|
| Scheduled → RollingOut | `InitiateRolloutAction` | `DurationRequired` reaches zero | `CompleteRolloutAction` |
| RollingOut → OnPad | — | (same action, fires on duration end) | `CompleteRolloutAction` |
| OnPad → Launched | `InitiateLaunchAction` | `today >= launch_date` | `CompleteLaunchAction` |
| Any → Cancelled | `CancelLaunchAction` | — | — |

`InitiateRolloutAction` calls `RocketMoveAction` internally, advancing the plan to `RollingOut` and the rocket to `Moving` in the same `execute()`. `CompleteRolloutAction` advances both to `OnPad` and `Stored` when the duration expires.

### Component-based rocket assembly (future)

If rockets are later assembled from discrete components — lower stage, upper stage, vacuum engine, sea-level engines, tanks, fairings — the key invariant is:

**A `LaunchPlan` always references a fully assembled rocket entity, never a frame or partial build.**

Manufacturing produces components as child entities. `IntegrateRocketAction` promotes the frame to a full `Rocket` in `Stored` state once all components are complete. Stats (`rollout_days`, `failure_rate`, `cost`) are derived from component stats during integration via the modifier system (ADR 002). `ScheduleLaunchAction` gains one additional precondition: no child components still under construction. Everything else in the launch plan flow is unaffected.

## UI Requirements

The active launches window must surface the timeline without requiring the player to open each plan:

- Columns: Mission, Current Stage, Rollout Date, Arrival Date, Launch Date, Rocket, Pad
- One manual trigger button per row for the current pending action
- `TransitionBlocked` reason shown inline in the row, not in a tooltip

## Phased Implementation

| Phase | Scope |
|---|---|
| 1 | `rollout_days` Stat on `Rocket`; `arrival_date` / `rollout_date` on `LaunchPlan`; recompute in `ScheduleLaunchAction` and `EditLaunchAction`; update pad conflict validation |
| 2 | Stage entities for `RollingOut` and `OnPad`; `InitiateRolloutAction` and `CompleteRolloutAction`; daily auto-progression system; manual trigger button in UI |
| 3 | `Launched` / `Cancelled` / `Failed` terminal stages; contract completion; notifications |
| 4 | `LaunchProfile` on rocket prefab for data-driven stage sequences |

## Non-Goals

- VAB / integration facility as an intermediate stop before rollout
- Fueling simulation
- Weather delays
- Pad-specific prep durations
- Multiple simultaneous blockers per plan
- Mod stage validate hooks in the UI (phase 4+ territory)
