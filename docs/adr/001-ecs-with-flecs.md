# 001 — ECS with Flecs

**Status:** Accepted

## Context

SolCorp needs to manage many heterogeneous game entities (celestial bodies, buildings, staff, rockets, contracts) with varying combinations of behaviour. A traditional object-oriented hierarchy would require deep inheritance trees and struggle with entities that share behaviour across unrelated domains (e.g. both a rocket and a text label need a `Transform`).

## Decision

Use an Entity Component System (ECS) architecture via [Flecs v4.1.2](https://www.flecs.dev/). All game state lives as components on entities; behaviour lives in systems that query for component combinations.

The game is structured as independent modules, each a C++ struct whose constructor registers its components and systems with the Flecs world. This keeps each domain self-contained.

## Consequences

- **Composable entities** — any entity can have any combination of components; no inheritance required.
- **Data-oriented** — components are stored in contiguous arrays; cache-friendly iteration.
- **Flecs relationships** — `ChildOf` and `IsA` (prefabs) replace explicit parent-ID fields and data duplication.
- **Phase scheduling** — Flecs pipelines enforce the 11-phase execution order without manual ordering code.
- **Coupling cost** — cross-module communication must go through ECS queries or shared component types, not direct function calls. This is intentional but requires discipline.
- **Learning curve** — Flecs has a rich API; developers unfamiliar with ECS may need time to internalise the query model.
