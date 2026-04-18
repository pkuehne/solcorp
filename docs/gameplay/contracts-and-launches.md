# Contracts & Launches

## Contracts

Contracts are offered by client companies and describe a mission to be performed. Each contract has:

| Field | Description |
|-------|-------------|
| Name | Mission name (e.g. "Launch Satellite") |
| Client | Company commissioning the launch |
| Description | Flavour text |
| Reward range | Minimum and maximum payment |
| Payload(s) | One or more payloads to be delivered |

### Payloads

Each payload specifies:
- A name (e.g. "Satellite 4231")
- A mass (kg)
- A target orbit path (e.g. `Sun::Earth::Low Orbit`)

## Launching

To execute a contract:
1. A compatible rocket must be available (supports the target orbit and has sufficient payload capacity)
2. A launchpad must be available at an active site
3. The launch is scheduled and transitions through the active launch pipeline

## Active Launches

In-progress launches are tracked as **active launch** entities. The game filters launches by status and site. *(Details TBD)*

## Orbital Paths

Orbits are addressed as hierarchical paths through the celestial body tree:

```
Sun::Earth::Low Orbit
Sun::Earth::Polar Orbit
Sun::Earth::Transfer Orbit
Sun::Earth::Synchronous Orbit
```
