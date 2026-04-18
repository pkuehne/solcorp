# Core Concepts

## Sites

A **site** is your launch complex — a grid of buildable tiles. You can own multiple sites. Each site has a name, dimensions, and an active flag.

Sites aggregate **effects** and **modifiers** that influence launch statistics (e.g. max payload weight, preparation days).

## Buildings

Buildings are placed on site tiles at a grid position. Each building is an instance of a **building prefab** (e.g. "Launch Complex", "Office Building", "Factory").

Buildings contain one or more **facilities** that provide specific capabilities.

## Facilities

| Facility | Purpose |
|----------|---------|
| Launchpad | Required to execute a rocket launch |
| Office | Provides administrative capacity |
| Storage | Stores manufactured components |
| Manufacturing | Produces rockets |

## Effects & Modifiers

**Effects** are named conditions attached to a site or building (e.g. "Better Concrete", "Cracks Detected"). Each effect carries one or more **modifiers** that adjust a named statistic multiplicatively or additively.

Example modifiers:

| Stat | Description |
|------|-------------|
| `max-weight` | Maximum payload mass the launchpad supports |
| `prep-days` | Days required to prepare a launch |

## Rockets

Rockets are instances of a **rocket prefab**. A prefab defines the rocket's name and its list of supported **target orbits** (e.g. Low Orbit, Polar Orbit) each with an associated payload capacity.

## Staff

Staff are personnel entities assigned to facilities. They affect operational capacity and launch readiness. *(Details TBD)*
