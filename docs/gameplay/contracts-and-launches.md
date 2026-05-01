# Contracts & Launches

## Contracts

Contracts are offered by client companies and describe a mission to be performed. To see the list of available contracts, open the "Contracts" window from the toolbar. Each contract will specify:
- A client company (e.g. "Acme Corp")
- A description of the mission (e.g. "Launch a communications satellite into low Earth orbit")
- The total payment for successful completion (e.g. $10 million)
- The Target Orbit (e.g. "Low Earth Orbit")
- A list of actions (Accept/Reject/Plan Launch) that can be taken on the contract

![Contracts Window](../assets/contracts-window.png "The contracts window showing a list of available contracts with details and rewards")

Contracts are generated periodically. Once accepted, the contract is added to your active contracts and the payloads become available to be scheduled onto a launch (See below).

An upfront payment is awarded for accepting the contract and the remaining reward is paid out upon successful completion of the contract (i.e. delivering the payload to the target orbit).

### Payloads

Each payload specifies:

- A name (e.g. "Satellite 4231")
- A mass (kg)
- A target orbit path (e.g. `Low Orbit`)

Payloads are tied to a contract and a site and can be attached to a rocket launch via a launch plan. The total mass of attached payloads must be within the rocket's payload capacity for the launch plan to be valid.

![Payload Launch](../assets/payload-launch.png "Example of payloads attached to a launch plan")

## Launch Planning

To launch a rocket, requires a Launch Plan. These can be created from a rocket, contract or launchpad. A launch plan specifies:
- A rocket to be launched
- A launchpad to launch from
- A target orbit to reach
- A launch date (which must be a few days in the future to allow for preparation)
- One or more payloads to be delivered (which must be compatible with the target orbit)

![Launch Plan](../assets/launch-plan.png "Example of a launch plan with rocket, launchpad, target orbit, and payloads")

The save button will indicate whether all pre-conditions for a valid launch plan are met. If not, it will show which conditions are missing (e.g. "Rocket does not have enough payload capacity for the selected payloads"). Once saved, the launch plan is added to the list of active launches and will be executed on the specified launch date.

## Active Launches

These can be seen in the "Active Launches" window.

![Active Launches](../assets/active-launches.png "Example of the active launches window showing upcoming launches")