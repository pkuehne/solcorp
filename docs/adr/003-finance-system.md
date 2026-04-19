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

## Entity Hierarchy

All financial state lives under a single `Company` entity. `Treasury` is itself an entity (not just a component) carrying the `Treasury` component. `Transactions` and `Categories` are well-known child entities of `Treasury`, providing stable roots for queries and a clean namespace in the Flecs REST inspector.

```mermaid
graph BT
    Company["Company"]
    Treasury["Treasury\n[Treasury component: balance]"]
    Transactions["Transactions\n[root for all tx entities]"]
    Categories["Categories\n[root for all category entities]"]

    TX1["tx_001\n[Transaction, Posted, Recurring]\nBelongsTo ──► Salary\nOriginatedFrom ──► Staff entity"]
    TX2["tx_002\n[Transaction, Posted]\nBelongsTo ──► Launch\nOriginatedFrom ──► Contract entity"]
    TX3["tx_003\n[Transaction]\nBelongsTo ──► Construction\nOriginatedFrom ──► Building entity"]

    Revenue["Revenue"]
    Launch["Launch"]
    OpEx["OpEx"]
    Salary["Salary"]
    Maintenance["Maintenance"]
    CapEx["CapEx"]
    Construction["Construction"]

    Treasury -->|ChildOf| Company
    Transactions -->|ChildOf| Treasury
    Categories -->|ChildOf| Treasury

    TX1 -->|ChildOf| Transactions
    TX2 -->|ChildOf| Transactions
    TX3 -->|ChildOf| Transactions

    Revenue -->|ChildOf| Categories
    Launch -->|ChildOf| Revenue
    OpEx -->|ChildOf| Categories
    Salary -->|ChildOf| OpEx
    Maintenance -->|ChildOf| OpEx
    CapEx -->|ChildOf| Categories
    Construction -->|ChildOf| CapEx
```

## Structure

### Company entity and Treasury

`Treasury` is a named entity under `Company`, carrying the `Treasury` component with the cached posted balance.

```cpp
struct Treasury {
  double balance = 0.0; // running total of posted transactions only
};

// Created by FinanceModule:
auto company  = world.entity("Company");
auto treasury = world.entity("Treasury").child_of(company).set<Treasury>({});
auto tx_root  = world.entity("Transactions").child_of(treasury);
auto cat_root = world.entity("Categories").child_of(treasury);
```

### Transaction component and tags

```cpp
struct Transaction {
  double      amount;       // positive = income, negative = expense
  std::string description;
  uint32_t    created_day;  // game day the transaction was created
  uint32_t    post_day;     // game day it becomes effective (= created_day for immediate)
};

struct Posted    {};  // applied once the transaction is reflected in Treasury.balance
struct Recurring {};  // marks transactions that originate from a repeating schedule
struct Failed    {};  // future-dated debit that could not post due to insufficient funds
```

Transactions are Flecs entities `ChildOf(tx_root)`. A system in `UpdatePhase` adds `Posted` and updates `Treasury.balance` when `post_day <= current_game_day`.

### Categorisation

Categories are named entities `ChildOf(cat_root)`, forming a two-level hierarchy (top-level group → specific type). Seeded by `FinanceModule` and extensible from Lua mods.

```
Categories
├── Revenue
│   └── Launch
├── OpEx
│   ├── Salary
│   └── Maintenance
└── CapEx
    ├── Construction
    └── Permit
```

Each transaction links to a category via the `BelongsTo` relationship:

```cpp
struct BelongsTo {};  // relationship: transaction → category entity

flecs::entity salary_cat = world.entity("Company::Treasury::Categories::OpEx::Salary");
tx_entity.add<BelongsTo>(salary_cat);
```

Querying all salary transactions:
```cpp
flecs::entity salary_cat = world.entity("Company::Treasury::Categories::OpEx::Salary");
world.query<Transaction>().with<BelongsTo>(salary_cat).build();
```

### Source linking

Each transaction optionally links back to the ECS entity that originated it (contract, building, staff team, etc.) via the `OriginatedFrom` relationship:

```cpp
struct OriginatedFrom {};  // relationship: transaction → source entity

tx_entity.add<OriginatedFrom>(contract_entity);
```

This enables "show me all spend on Building X" or "show me all revenue from Contract Y" as plain Flecs queries without any extra bookkeeping.

### Recurring / one-off

The `Recurring` tag marks transactions created by a repeating schedule (monthly salaries, building maintenance). One-off transactions (construction cost, permit fee) carry no tag. A scheduling system creates the next `Recurring` transaction entity when the previous one posts.

### Action pattern

Financial mutations go through the action interface from the rocket launch module (to be formalised in a separate ADR):

```cpp
struct TransactAction : IAction {
  double      amount;          // positive = income, negative = expense
  std::string description;
  uint32_t    post_day = 0;    // 0 = current game day (immediate)
  flecs::entity category;      // BelongsTo target
  flecs::entity origin;        // OriginatedFrom target (optional, may be null)
  bool          recurring = false;

  ValidationResult validate(const flecs::world& world) const override;
  // For immediate debits: checks Treasury.balance + amount >= 0.
  // For future-dated debits: checks projected balance
  // (Treasury.balance + sum of all non-Failed pending transactions + amount >= 0).

  void execute(flecs::world& world) override;
  // Creates a Transaction entity ChildOf(tx_root) with BelongsTo(category),
  // optionally OriginatedFrom(origin), and Recurring tag if set.
  // If post_day <= current_game_day, adds Posted and updates Treasury.balance immediately.
};
```

Callers check `validate()` before showing confirmation UI or scheduling the action. The posting system also validates at post time; if the balance has fallen since scheduling, the transaction receives `Failed` rather than silently overdrafting.

### Contract integration

The existing `Contract` component's `upfront_payment` and `completion_payment` floats are unchanged — they are data, not behaviour. The rocket launch module calls `TransactAction` at the appropriate moments:

- Upfront payment: immediate `TransactAction` with `category = launch_cat` and `origin = contract_entity`, called when status moves to `Accepted`.
- Completion payment: future-dated `TransactAction` with the same category and origin, created at acceptance so it appears in cashflow projections before the launch occurs.

## Extensibility

The following are not implemented now but are supported by this model without structural changes:

- **Cashflow projection** — query all `Transaction` entities lacking `Posted` (and not `Failed`) within a date range; sum against current balance.
- **Money market / investments** — a new action type creates a pair of transactions: an immediate debit (a new `CapEx` leaf category) and a future-dated credit (a new `Revenue` leaf category), both with the same `OriginatedFrom` origin entity.
- **Multiple companies** — `Treasury`, `Transactions`, and `Categories` are all `ChildOf` a specific `Company` entity; a second company gets its own subtree and its own `Treasury` component instance.
- **Save-game verification** — recompute `Treasury.balance` by summing all `Posted` transactions and assert it matches the cached value.
- **New categories from mods** — Lua calls `world.entity("MyNewCost").child_of(world.entity("Company::Treasury::Categories::OpEx"))` to register a new leaf category; no C++ changes required.

## Consequences

- **Audit trail** — every financial event is a queryable Flecs entity; a ledger view queries `Transaction` children of the `Transactions` entity.
- **Overdraft prevention** — `TransactAction::validate()` blocks debits that would go negative; the validate/execute split lets UI show errors before committing.
- **Future posting** — contract completions, salaries, and maintenance are first-class pending entities rather than ad-hoc callbacks.
- **Categorised spend** — `BelongsTo` relationships enable per-category rollups (monthly salary total, total construction spend) as plain Flecs queries.
- **Source traceability** — `OriginatedFrom` links each transaction to its cause without storing entity IDs in component fields.
- **ECS-native** — Flecs handles thousands of transaction entities efficiently; no separate data structure needed.
- **Cached balance** — `Treasury.balance` avoids summing all transactions each frame; full recomputation is reserved for verification.
