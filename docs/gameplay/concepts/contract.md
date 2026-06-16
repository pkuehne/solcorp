# Contract

**Contracts** are offered by client companies and describe a mission to be performed. To see the list of available contracts, open the "Contracts" window from the toolbar. In the window you will see the list of available contracts. Click on the "view" button to see the details of a contract and decide whether to accept or reject it.

Each contract will specify:
- A client company (e.g. "Acme Corp")
- A description of the mission (e.g. "Launch a communications satellite into low Earth orbit")
- The total payment for successful completion (e.g. $10 million)
- The Target Orbit (e.g. "Low Earth Orbit")
- Buttons to accept or reject the contract

![Contracts Window](../../assets/contracts-window.png "The contracts window showing a list of available contracts with details and rewards")

Contracts are generated periodically. Once accepted, the contract is added to your active contracts and the payloads become available to be scheduled onto a launch (See below).

An upfront payment is awarded for accepting the contract and the remaining reward is paid out upon successful completion of the contract (i.e. delivering the payload to the target orbit).


### Payloads

Each payload specifies:

- A name (e.g. "Satellite 4231")
- A mass (kg)
- A target orbit path (e.g. `Low Orbit`)

Payloads are tied to a contract and a [site](gameplay/concepts/site.md) and can be attached to a rocket launch via a launch plan. The total mass of attached payloads must be within the rocket's payload capacity for the launch plan to be valid.

![Payload Launch](../../assets/payload-launch.png "Example of payloads attached to a launch plan")
