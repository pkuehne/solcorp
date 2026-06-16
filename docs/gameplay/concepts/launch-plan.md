# Launch Plan

To launch a rocket, you need a **Launch Plan**. Plans can be created from a [rocket](gameplay/concepts/rocket.md), a [contract](gameplay/concepts/contract.md), or a launchpad. A launch plan specifies:

- A [rocket](gameplay/concepts/rocket.md) to be launched
- A launchpad to launch from
- A target orbit to reach
- One or more payloads to be delivered (which must be compatible with the target orbit)

The launch date is calculated automatically based on the time needed to move the rocket and prepare it at the pad.

![Launch Plan](../../assets/launch-plan.png "Example of a launch plan with rocket, launchpad, target orbit, and payloads")

Once saved, the launch plan is added to the list of active launches and will execute automatically on the scheduled date.

## Stages

Launch plans progress through several stages automatically:

| Stage | Description |
|-------|-------------|
| **Rollout** | The rocket is moved from storage to the launchpad |
| **Pad Prep** | The rocket is fuelled and undergoes final checks |
| **Launch** | The rocket lifts off and ascends to orbit |

## Outcomes

There is a small chance the launch fails. The failure rate is determined by the rocket's stats and any active effects. If the launch fails, the rocket is destroyed and any attached payloads are lost. If it succeeds, the payloads are delivered to the target orbit and the contract is completed, awarding the remaining payment.

## Managing Launch Plans

Use the [Launch Plan](gameplay/windows/launch-plan.md) window to create or edit a plan, and the [Active Launches](gameplay/windows/active-launches.md) window to monitor all scheduled and in-progress launches.
