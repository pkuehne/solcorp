# Flecs Relationships

Flecs relationships allow entities to be linked without coupling their components. SolCorp uses several relationships to express game structure.

## Built-in Relationships

### `ChildOf`

Parent-child ownership. Used for:

| Parent | Child | Meaning |
|--------|-------|---------|
| Site entity | Building entity | Building belongs to site |
| Building entity | Facility entity | Facility is inside a building |
| Building entity | Text/effect entity | Visual or gameplay overlay on a building |
| Celestial body | Moon/orbit | Orbital hierarchy (Sun → Earth → Moon) |

Querying children:

```cpp
world.query<Transform>()
    .with(flecs::ChildOf, site_entity)
    .build();
```

### `IsA` (Prefabs)

Prefab inheritance. Rocket and building *instances* inherit from their prefab via `IsA`. Component values on the prefab are shared unless overridden on the instance.

```cpp
flecs::entity rocket_instance = world.entity()
    .is_a(falcon1_prefab);
```

## Custom Relationships

These are documented in the respective module docs, but examples include:

- `LaunchedFrom` (RocketLaunchModule) — links a rocket to its launch site
- `AssignedTo` (StaffModule) — links personnel to their assigned site or facility   

## Relationship Tips

- Prefer `ChildOf` over a "parent id" component — Flecs can query and cascade-delete children automatically.
- Prefabs with `IsA` avoid duplicating component data across many instances.
- Use the Flecs REST API (`localhost:27750`) to inspect live entity relationships during development.
