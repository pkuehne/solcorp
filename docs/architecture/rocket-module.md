# Rocket Module

The Rocket Module manages rocket construction, launch contracts, and launch operations. It defines components for rockets, launchpads, and contracts, as well as systems for building rockets, accepting contracts, and launching.

## Key Components
- `Rocket` — defines the necessary components and stats for a rocket. Not to be confused with specific rocket prefabs, which are templates for individual rocket types (e.g. Falcon 1).
- `LaunchPlan` - defines the core parameters to execute a launch, including the rocket, payload, and target orbit.
- `Payload` — A tag that defines what can be launched aboard a rocket
- `Contract` — defines a launch contract with requirements and rewards

## Relationships

Relationships are symmetric, so if a `LaunchPlan` has a `LaunchingFrom` relationship to a launch site, that site can query for all plans launching from it. This allows for flexible querying and system design without tight coupling between entities.

The following relationships link a `LaunchPlan` to other entities:

- `LaunchingFrom` — links a plan to its launch site (a building with a launchpad)
- `LaunchingWith` — links a plan to its `Payload`
- `LaunchingOn` - links a plan to its `Rocket` instance

The `Rocket` has the following relationships:

- `LaunchingOn` - links a rocket to its `LaunchPlan` (if currently assigned to one)
- `CanLiftTo` - links a rocket prefab to the `TargetOrbit`s it can reach, used for contract matching. The relationship also defines the amount of mass the rocket can lift to that orbit.

A `Contract` has the following relationships:

- `ContractPayload` — links a contract to the `Payload` it requires
- `ContractTargetOrbit` — links a contract to the `TargetOrbit` for the launch