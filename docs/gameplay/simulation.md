# Simulation

## Celestial Bodies

The simulation models a hierarchical solar system. Celestial bodies are ECS entities with orbital parameters. The hierarchy (e.g. Sun → Earth → Moon) is expressed through Flecs parent-child relationships.

## Orbital Mechanics

Orbital calculations determine body positions over time. Bodies have:
- Semi-major axis
- Eccentricity
- Orbital period
- Current mean anomaly

The simulation phase advances each body's position each tick.

## Developer Tools

The **Developer Window** (accessible in-game) exposes live orbital state for debugging. The Flecs REST API (enabled at runtime) allows inspection of all simulation entities.

*(Details TBD — expand as the simulation module grows)*
