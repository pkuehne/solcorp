#include "modules/base/notification.h"
#include "modules/rocket/launch_actions.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

SCENARIO("LaunchGoAction", "[action]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();
  createNotificationCategory(world, "Contracts");

  GIVEN("An invalid plan") {
    LaunchGoAction action;

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not valid");
      }
    }
  }

  GIVEN("A plan not in OnPad state") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(
        world.lookup("States::LaunchPlan::RollingOut"));
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan is not in preparation");
      }
    }
  }

  GIVEN("An OnPad plan with unfinished preparation") {
    auto plan = world.entity().set<LaunchPlan>({});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    plan.set<DurationRequired>({.remaining = 3, .total = 5});
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch preparation is not yet complete");
      }
    }
  }

  GIVEN("An OnPad plan with no rocket assigned") {
    uint32_t today = world.get<Game>().day;
    auto plan = world.entity().set<LaunchPlan>({.launch_date = today});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch plan has no rocket assigned");
      }
    }
  }

  GIVEN("An OnPad plan where the launch date has not yet arrived") {
    uint32_t today = world.get<Game>().day;
    auto rocket = world.entity().add<Rocket>();
    auto plan = world.entity().set<LaunchPlan>({.launch_date = today + 1});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    plan.add<LaunchingOn>(rocket);
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It fails") {
        CHECK(!result.ok);
        CHECK(result.message == "Launch date has not yet arrived");
      }
    }
  }

  GIVEN("An OnPad plan ready to launch") {
    uint32_t today = world.get<Game>().day;
    auto targetOrbit = world.entity("LEO");
    auto launchpad = world.entity("Main Pad").add<Launchpad>();
    auto rocket = world.entity("Falcon 9").add<Rocket>();
    // Post-rollout the rocket physically sits on the launchpad
    rocket.child_of(launchpad);
    rocket.get_mut<Rocket>().failure_rate.setBase(0.0);
    auto plan = world.entity("Test Plan")
                    .set<LaunchPlan>(
                        {.launch_date = today, .target_orbit = targetOrbit});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    LaunchGoAction action{plan};

    WHEN("Validated") {
      ValidationResult result = action.validate(world);

      THEN("It succeeds") {
        CHECK(result.ok);
        CHECK(result.message == "");
      }
    }

    WHEN("Executed") {
      action.execute(world);

      THEN("The rocket is retained in its Launched terminal state, moved to "
           "the target orbit, and the plan transitions to Launched") {
        CHECK(rocket.is_alive());
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Launched")));
        CHECK(rocket.parent() == targetOrbit);
        CHECK(plan.has<LaunchingOn>(rocket));
        CHECK(plan.is_alive());
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Launched")));
      }
    }
  }

  GIVEN("An OnPad plan with payloads and matching contract orbits") {
    uint32_t today = world.get<Game>().day;
    auto targetOrbit = world.entity("LEO");
    auto launchpad = world.entity("Pad A").add<Launchpad>();
    auto rocket = world.entity("Rocket A").add<Rocket>();
    rocket.get_mut<Rocket>().failure_rate.setBase(0.0);
    auto contractA = world.entity("Contract A1")
                         .set<Contract>({.name = "AlphaSat Delivery",
                                         .client = "Client",
                                         .description = "Desc",
                                         .upfront_payment = 1000u,
                                         .completion_payment = 2000u})
                         .add<ContractCurrentState>(
                             world.lookup("States::Contract::Accepted"));
    auto contractB = world.entity("Contract A2")
                         .set<Contract>({.name = "BetaSat Delivery",
                                         .client = "Client",
                                         .description = "Desc",
                                         .upfront_payment = 1000u,
                                         .completion_payment = 3000u})
                         .add<ContractCurrentState>(
                             world.lookup("States::Contract::Accepted"));
    contractA.add<ContractTargetOrbit>(targetOrbit);
    contractB.add<ContractTargetOrbit>(targetOrbit);
    auto payloadA = world.entity("Payload A1").set<Payload>({1000});
    auto payloadB = world.entity("Payload A2").set<Payload>({2000});
    contractA.add<ContractPayload>(payloadA);
    contractB.add<ContractPayload>(payloadB);
    auto plan = world.entity("Plan A").set<LaunchPlan>(
        {.launch_date = today, .target_orbit = targetOrbit});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    plan.add<LaunchingWith>(payloadA);
    plan.add<LaunchingWith>(payloadB);
    LaunchGoAction action{plan};

    WHEN("Executed") {
      action.execute(world);

      THEN("The rocket and payloads are retained, moved to the target orbit, "
           "and the plan transitions to Launched") {
        CHECK(rocket.is_alive());
        CHECK(rocket.has<RocketCurrentState>(
            world.lookup("States::Rocket::Launched")));
        CHECK(rocket.parent() == targetOrbit);
        CHECK(payloadA.is_alive());
        CHECK(payloadB.is_alive());
        CHECK(payloadA.parent() == targetOrbit);
        CHECK(payloadB.parent() == targetOrbit);
        CHECK(plan.has<LaunchingOn>(rocket));
        CHECK(plan.has<LaunchingWith>(payloadA));
        CHECK(plan.has<LaunchingWith>(payloadB));
        CHECK(plan.is_alive());
        CHECK(plan.has<LaunchPlanCurrentState>(
            world.lookup("States::LaunchPlan::Launched")));
      }
      THEN("Contracts are closed and completion payments are collected") {
        CHECK(contractA.has<ContractCurrentState>(
            world.lookup("States::Contract::Closed")));
        CHECK(contractA.get<Contract>().failed == false);
        CHECK(contractB.has<ContractCurrentState>(
            world.lookup("States::Contract::Closed")));
        CHECK(contractB.get<Contract>().failed == false);
        CHECK(world.get<Company>().balance == 5000);
      }
      THEN("Contract completion notifications are created") {
        int matched = 0;
        world.query_builder<Notification>().build().each(
            [&](flecs::entity notifE, Notification &notification) {
              if (notification.title != "Contract Completed") {
                return;
              }
              CHECK(notifE.has<NotificationCategory>(
                  world.lookup("NotificationCategories::Contracts")));
              if (notification.text ==
                  "'AlphaSat Delivery' has been completed and paid out "
                  "$2,000") {
                matched++;
              }
              if (notification.text ==
                  "'BetaSat Delivery' has been completed and paid out $3,000") {
                matched++;
              }
            });
        CHECK(matched == 2);
      }
    }
  }

  GIVEN("An OnPad plan launched to the wrong orbit") {
    uint32_t today = world.get<Game>().day;
    auto launchedOrbit = world.entity("LEO");
    auto contractOrbit = world.entity("GTO");
    auto launchpad = world.entity("Pad B").add<Launchpad>();
    auto rocket = world.entity("Rocket B").add<Rocket>();
    rocket.get_mut<Rocket>().failure_rate.setBase(0.0);
    auto contract = world.entity("Contract B1")
                        .set<Contract>({.name = "GTO Delivery",
                                        .client = "Client",
                                        .description = "Desc",
                                        .upfront_payment = 1000u,
                                        .completion_payment = 2000u})
                        .add<ContractCurrentState>(
                            world.lookup("States::Contract::Accepted"));
    contract.add<ContractTargetOrbit>(contractOrbit);
    auto payload = world.entity("Payload B1").set<Payload>({1000});
    contract.add<ContractPayload>(payload);
    auto plan = world.entity("Plan B").set<LaunchPlan>(
        {.launch_date = today, .target_orbit = launchedOrbit});
    plan.add<LaunchPlanCurrentState>(world.lookup("States::LaunchPlan::OnPad"));
    plan.add<LaunchingOn>(rocket);
    plan.add<LaunchingFrom>(launchpad);
    plan.add<LaunchingWith>(payload);
    LaunchGoAction action{plan};

    WHEN("Executed") {
      action.execute(world);

      THEN("The contract is marked failed and closed") {
        CHECK(contract.has<ContractCurrentState>(
            world.lookup("States::Contract::Closed")));
        CHECK(contract.get<Contract>().failed == true);
      }
      THEN("No payment is added to the company balance") {
        CHECK(world.get<Company>().balance == 0);
      }
      THEN("A contract failure notification is created") {
        bool found = false;
        world.query_builder<Notification>().build().each(
            [&](flecs::entity notifE, Notification &notification) {
              if (notification.title != "Contract Failed") {
                return;
              }
              CHECK(notifE.has<NotificationCategory>(
                  world.lookup("NotificationCategories::Contracts")));
              CHECK(notification.severity == NotificationSeverity::Important);
              if (notification.text == "'GTO Delivery' has failed") {
                found = true;
              }
            });
        CHECK(found);
      }
    }
  }
}
