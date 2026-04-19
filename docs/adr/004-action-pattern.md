# 004 — Action Pattern for Validated World Mutations

**Status:** Accepted

## Context

The game needs UI-driven operations that mutate ECS world state — scheduling launches, moving rockets, posting transactions. These share a common shape:

- The user builds up intent over multiple UI interactions (selecting a rocket, a launchpad, a date)
- The operation has preconditions that must all pass before the mutation is safe
- The UI should communicate exactly *why* an operation is blocked
- The logic must be testable without rendering any ImGui widgets

Inlining validation and mutation inside draw functions solves none of these: the logic is untestable, the error messages are ad-hoc, and the UI state is entangled with world state.

## When to use this pattern

Use an action struct when **all three** of the following are true:

1. The operation mutates ECS world state
2. It has preconditions that can fail, and the UI must explain why
3. The user builds up the inputs before committing (draft state)

Direct mutations (e.g. toggling a flag, incrementing a counter on button click) do not need an action struct — the overhead is not justified when there is nothing to validate and no draft state to hold.

## Options Considered

### Option 1 — Inline validation in draw functions

Validation logic and mutation written directly inside the ImGui draw function. Simple to add initially, but the logic becomes untestable, error messages are ad-hoc strings scattered across draw code, and adding a new check requires finding the right place in a long render function.

### Option 2 — Free `check_*` functions

Standalone functions like `bool check_launch_valid(const LaunchPlan&, const flecs::world&)` called from the draw function. More testable than option 1, but each operation accumulates its own bespoke function signature, return type, and calling convention. There is no shared contract between operations, so UI code cannot be written generically against them, and the link between "what was checked" and "what gets executed" is implicit.

### Option 3 — `IAction` with `validate()` / `execute()` (chosen)

A single interface that all operations implement. The contract is explicit: validate reads, execute mutates. `ValidationResult` is a shared type the UI already knows how to display. New operations are consistent by construction.

## Decision

Operations that mutate world state are represented as **action structs** implementing `IAction`, defined in `src/modules/base/action.h`:

```cpp
struct IAction {
  virtual ValidationResult validate(const flecs::world &world) const = 0;
  virtual void execute(flecs::world &world) = 0;
};
```

**`validate()`** is read-only. It checks all preconditions and returns a `ValidationResult`:

```cpp
struct ValidationResult {
  bool ok = false;
  std::string message;

  static ValidationResult Pass()                        { return {true, {}}; }
  static ValidationResult Fail(const std::string &msg)  { return {false, msg}; }
  explicit operator bool() const noexcept               { return ok; }
};
```

On failure, `message` is a human-readable explanation suitable for display verbatim in the UI.

**`execute()`** is called only after validation passes and the user confirms. It performs all world mutations. Complex actions (e.g. editing an existing plan) re-validate defensively at the start of `execute()`.

**UI state** holds the draft action as a plain struct member (e.g. `LaunchWindow::draftPlan`). ImGui controls mutate the struct's fields directly. The `ActionButton` widget consumes the validation result:

```cpp
auto valid = action.validate(world);
if (ActionButton("Save", "Save plan", valid.message)) {
    action.execute(world);
}
```

`ActionButton` disables the button and shows the failure message as a red tooltip when `valid.message` is non-empty — no separate error dialog required.

## Consequences

- **Testable:** `validate()` and `execute()` take a `flecs::world&` and nothing else. Tests construct a world, set up entities, call the action, and assert on results — no UI harness needed.
- **Clear UX:** every rejection path must produce a specific, actionable message. Vague errors are a code smell caught at review time.
- **Separation of concerns:** validation never mutates; execution never re-derives preconditions (except as a defensive guard).
- **Boilerplate per action:** each new operation requires a struct, a `validate()`, and an `execute()`. This is intentional — it forces explicit modelling of preconditions rather than burying them in draw code.
- **No async or deferred execution:** `execute()` runs synchronously on the main thread during the GUI phase. Actions that need deferred effects (e.g. posting a transaction on a future day) model that by creating an ECS entity, not by deferring `execute()` itself.

## Examples in the Codebase

- `ScheduleLaunchAction` — 11 validation checks, creates `LaunchPlan` entity with relationships; editing destroys and recreates the plan
- `MoveRocketAction` — 4 validation checks, reparents a rocket entity to a destination facility
