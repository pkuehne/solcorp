# Launch Plan

To launch a rocket, requires a Launch Plan. These can be created from a rocket, contract or launchpad. A launch plan specifies:

- A rocket to be launched
- A launchpad to launch from
- A target orbit to reach
- A launch date (which must be a few days in the future to allow for preparation)
- One or more payloads to be delivered (which must be compatible with the target orbit)

![Launch Plan](../assets/launch-plan.png "Example of a launch plan with rocket, launchpad, target orbit, and payloads")

The save button will indicate whether all pre-conditions for a valid launch plan are met. If not, it will show which conditions are missing (e.g. "Rocket does not have enough payload capacity for the selected payloads"). Once saved, the launch plan is added to the list of active launches and will be executed on the specified launch date.

Launch plans go through several stages. Clicking on "view" for a planned launch will show the details of the plan as well as the progress through the stages. The stages include:
- **Move** — The rocket is moved to the launchpad from its current location (e.g. factory or storage)
- **Rollout** — The rocket is prepared for launch at the launchpad (e.g. fueling, final checks)
- **Launch** — The rocket is launched and begins its ascent to orbit

When the rocket launches there is a small change it will fail to launch successfully. The failure rate is determined by the rocket's stats and any active effects. If the launch fails, the rocket is destroyed and any attached payloads are lost. If the launch succeeds, the payloads are delivered to the target orbit and the contract is completed, awarding the remaining payment.
