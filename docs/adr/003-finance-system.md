# 003 — Finance System

**Status:** Proposed

## Context

The game needs to track company finances. The simplest implementation — a single float balance mutated directly by any system — has several problems:

- No audit trail: impossible to know why the balance changed
- No prevention of overdrafts: any system can write any value
- No future-dated transactions: upfront vs. completion contract payments require deferred posting
- No foundation for cashflow projections or more complex instruments later

The `Contract` component already carries raw `float upfront_payment` and `float completion_payment` fields with no mechanism to post them, which means no current integration with any balance concept.

## Options Considered

### Option 1 — Singleton balance float

A `Treasury` singleton holds a `double balance`. Any system calls a free function to add or subtract. Simple but provides no audit trail, no validation, no deferred posting.

### Option 2 — Append-only transaction log, recomputed balance

Every financial event appends to a log (vector or similar). Current balance = sum of all posted entries. Auditable, but recomputing from a full log each frame is wasteful, and the log lives outside ECS.

### Option 3 — Transaction entities + cached balance + action validation (chosen)

Each financial event is a Flecs entity. A `Treasury` singleton caches the running balance. Changes go through an `IAction` subclass that validates before executing. Future-dated transactions are entities with a post date; a system promotes them to posted when game time arrives.

## Decision

**Option 3.** The transaction-as-entity model is a natural fit for Flecs (thousands of lightweight entities), gives a queryable audit trail, and integrates with the existing `IAction` validate/execute pattern already in use in the rocket launch module. A separate ADR will formalise the `IAction` pattern game-wide; this ADR assumes it.

See [Finance System design doc](../design/finance-system.md) for the full entity hierarchy, component definitions, and integration details.

## Consequences

- **Audit trail** — every financial event is a queryable Flecs entity; a ledger view queries `Transaction` children of the `Transactions` entity.
- **Overdraft prevention** — `TransactAction::validate()` blocks debits that would go negative; the validate/execute split lets UI show errors before committing.
- **Future posting** — contract completions, salaries, and maintenance are first-class pending entities rather than ad-hoc callbacks.
- **Categorised spend** — `BelongsTo` relationships enable per-category rollups (monthly salary total, total construction spend) as plain Flecs queries.
- **Source traceability** — `OriginatedFrom` links each transaction to its cause without storing entity IDs in component fields.
- **ECS-native** — Flecs handles thousands of transaction entities efficiently; no separate data structure needed.
- **Cached balance** — `Treasury.balance` avoids summing all transactions each frame; full recomputation is reserved for verification.
