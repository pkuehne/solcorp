#include "modules/rocket/active_launches_window.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include "modules/window/window_module.h"
#include <catch2/catch_test_macros.hpp>
#include <flecs.h>

struct ActiveLaunchesFixture {
  flecs::world world;

  flecs::entity site1;
  flecs::entity site2;
  flecs::entity pad1;
  flecs::entity pad2;

  flecs::entity orbit1;
  flecs::entity orbit2;

  flecs::entity payload1;
  flecs::entity payload2;

  flecs::entity plan1;
  flecs::entity plan2;
  flecs::entity plan3;

  ActiveLaunchesFixture() {
    world.import <SimulationModule>();
    world.import <WindowModule>();
    world.import <SiteModule>();
    world.import <RocketModule>();

    site1 = world.entity("Site A").add<Site>();
    site2 = world.entity("Site B").add<Site>();

    auto building1 = world.entity("Pad Building 1").child_of(site1);
    auto building2 = world.entity("Pad Building 2").child_of(site2);
    pad1 = world.entity().add<Launchpad>().add<Facility>().child_of(building1);
    pad2 = world.entity().add<Launchpad>().add<Facility>().child_of(building2);

    orbit1 = world.entity().set<TargetOrbit>(
        {.altitude = 200000.0, .inclination = 0.0});
    orbit2 = world.entity().set<TargetOrbit>(
        {.altitude = 400000.0, .inclination = 1.5708});

    payload1 = world.entity().set<Payload>({.mass = 500u});
    payload2 = world.entity().set<Payload>({.mass = 800u});

    world.entity()
        .set<Contract>({.client = "Client A",
                        .description = "Desc",
                        .upfront_payment = 1000u,
                        .completion_payment = 5000u,
                        .status = ContractStatus::Open,
                        .failed = false})
        .add<ContractPayload>(payload1)
        .add<ContractTargetOrbit>(orbit1);
    world.entity()
        .set<Contract>({.client = "Client B",
                        .description = "Desc",
                        .upfront_payment = 2000u,
                        .completion_payment = 8000u,
                        .status = ContractStatus::Open,
                        .failed = false})
        .add<ContractPayload>(payload2)
        .add<ContractTargetOrbit>(orbit2);

    plan1 = world.entity("Plan Alpha")
                .set<LaunchPlan>({.launch_date = 10u})
                .add<LaunchingFrom>(pad1)
                .add<LaunchingWith>(payload1);
    plan2 = world.entity("Plan Beta")
                .set<LaunchPlan>({.launch_date = 20u})
                .add<LaunchingFrom>(pad2)
                .add<LaunchingWith>(payload2);
    plan3 = world.entity("Plan Gamma")
                .set<LaunchPlan>({.launch_date = 30u})
                .add<LaunchingFrom>(pad1);
  }
};

SCENARIO("planMatchesFilters - no filters", "[filter][active_launches]") {
  ActiveLaunchesFixture f;

  GIVEN("An empty filter state") {
    ActiveLaunchesWindow state;

    THEN("All plans pass") {
      CHECK(planMatchesFilters(f.plan1, state));
      CHECK(planMatchesFilters(f.plan2, state));
      CHECK(planMatchesFilters(f.plan3, state));
    }
  }
}

SCENARIO("planMatchesFilters - site filter", "[filter][active_launches]") {
  ActiveLaunchesFixture f;

  GIVEN("filterSite = site1") {
    ActiveLaunchesWindow state;
    state.filterSite = f.site1;

    THEN("Plans on site1 pass") {
      CHECK(planMatchesFilters(f.plan1, state));
      CHECK(planMatchesFilters(f.plan3, state));
    }
    THEN("Plans on site2 are excluded") {
      CHECK(!planMatchesFilters(f.plan2, state));
    }
  }

  GIVEN("filterSite = site2") {
    ActiveLaunchesWindow state;
    state.filterSite = f.site2;

    THEN("Plans on site2 pass") { CHECK(planMatchesFilters(f.plan2, state)); }
    THEN("Plans on site1 are excluded") {
      CHECK(!planMatchesFilters(f.plan1, state));
      CHECK(!planMatchesFilters(f.plan3, state));
    }
  }
}

SCENARIO("planMatchesFilters - pad filter", "[filter][active_launches]") {
  ActiveLaunchesFixture f;

  GIVEN("filterPad = pad1") {
    ActiveLaunchesWindow state;
    state.filterPad = f.pad1;

    THEN("Plans on pad1 pass") {
      CHECK(planMatchesFilters(f.plan1, state));
      CHECK(planMatchesFilters(f.plan3, state));
    }
    THEN("Plans on pad2 are excluded") {
      CHECK(!planMatchesFilters(f.plan2, state));
    }
  }

  GIVEN("filterPad = pad2") {
    ActiveLaunchesWindow state;
    state.filterPad = f.pad2;

    THEN("Plans on pad2 pass") { CHECK(planMatchesFilters(f.plan2, state)); }
    THEN("Plans on pad1 are excluded") {
      CHECK(!planMatchesFilters(f.plan1, state));
      CHECK(!planMatchesFilters(f.plan3, state));
    }
  }
}

SCENARIO("planMatchesFilters - orbit filter", "[filter][active_launches]") {
  ActiveLaunchesFixture f;

  GIVEN("filterOrbit = orbit1") {
    ActiveLaunchesWindow state;
    state.filterOrbit = f.orbit1;

    THEN("plan1 (targeting orbit1 via its contract) passes") {
      CHECK(planMatchesFilters(f.plan1, state));
    }
    THEN("plan2 (targeting orbit2) is excluded") {
      CHECK(!planMatchesFilters(f.plan2, state));
    }
    THEN("plan3 (no payload, no orbit) is excluded") {
      CHECK(!planMatchesFilters(f.plan3, state));
    }
  }

  GIVEN("filterOrbit = orbit2") {
    ActiveLaunchesWindow state;
    state.filterOrbit = f.orbit2;

    THEN("plan2 (targeting orbit2 via its contract) passes") {
      CHECK(planMatchesFilters(f.plan2, state));
    }
    THEN("plan1 and plan3 are excluded") {
      CHECK(!planMatchesFilters(f.plan1, state));
      CHECK(!planMatchesFilters(f.plan3, state));
    }
  }
}

SCENARIO("planMatchesFilters - combined filters", "[filter][active_launches]") {
  ActiveLaunchesFixture f;

  GIVEN("filterSite = site1 and filterOrbit = orbit1") {
    ActiveLaunchesWindow state;
    state.filterSite = f.site1;
    state.filterOrbit = f.orbit1;

    THEN("plan1 (site1, orbit1) passes") {
      CHECK(planMatchesFilters(f.plan1, state));
    }
    THEN("plan2 (site2, orbit2) is excluded") {
      CHECK(!planMatchesFilters(f.plan2, state));
    }
    THEN("plan3 (site1, no orbit) is excluded") {
      CHECK(!planMatchesFilters(f.plan3, state));
    }
  }

  GIVEN("filterPad = pad1 and filterOrbit = orbit1") {
    ActiveLaunchesWindow state;
    state.filterPad = f.pad1;
    state.filterOrbit = f.orbit1;

    THEN("plan1 (pad1, orbit1) passes") {
      CHECK(planMatchesFilters(f.plan1, state));
    }
    THEN("plan3 (pad1, no orbit) is excluded") {
      CHECK(!planMatchesFilters(f.plan3, state));
    }
    THEN("plan2 (pad2, orbit2) is excluded") {
      CHECK(!planMatchesFilters(f.plan2, state));
    }
  }
}

SCENARIO("planMatchesFilters - showCompleted", "[filter][active_launches]") {
  ActiveLaunchesFixture f;

  GIVEN("A plan in a terminal (Launched) state") {
    f.plan1.add<LaunchPlanCurrentState>(
        f.world.lookup("States::LaunchPlan::Launched"));

    WHEN("showCompleted is false (default)") {
      ActiveLaunchesWindow state;

      THEN("The launched plan is excluded") {
        CHECK(!planMatchesFilters(f.plan1, state));
      }
      THEN("Non-terminal plans still pass") {
        CHECK(planMatchesFilters(f.plan2, state));
        CHECK(planMatchesFilters(f.plan3, state));
      }
    }

    WHEN("showCompleted is true") {
      ActiveLaunchesWindow state;
      state.showCompleted = true;

      THEN("The launched plan passes") {
        CHECK(planMatchesFilters(f.plan1, state));
      }
    }
  }
}

SCENARIO("planMatchesFilters - dead filter entities",
         "[filter][active_launches]") {
  ActiveLaunchesFixture f;

  GIVEN("filterSite is set then the site entity is destroyed") {
    auto filterSite = f.world.entity("Temp Site").add<Site>();
    ActiveLaunchesWindow state;
    state.filterSite = filterSite;
    filterSite.destruct();

    THEN("All plans pass (dead filter = no filter)") {
      CHECK(planMatchesFilters(f.plan1, state));
      CHECK(planMatchesFilters(f.plan2, state));
      CHECK(planMatchesFilters(f.plan3, state));
    }
  }

  GIVEN("filterPad is set then the pad entity is destroyed") {
    auto tempBuilding = f.world.entity().child_of(f.site1);
    auto filterPad = f.world.entity().add<Launchpad>().add<Facility>().child_of(
        tempBuilding);
    ActiveLaunchesWindow state;
    state.filterPad = filterPad;
    filterPad.destruct();

    THEN("All plans pass (dead filter = no filter)") {
      CHECK(planMatchesFilters(f.plan1, state));
      CHECK(planMatchesFilters(f.plan2, state));
      CHECK(planMatchesFilters(f.plan3, state));
    }
  }

  GIVEN("filterOrbit is set then the orbit entity is destroyed") {
    auto filterOrbit = f.world.entity().set<TargetOrbit>(
        {.altitude = 300000.0, .inclination = 0.5});
    ActiveLaunchesWindow state;
    state.filterOrbit = filterOrbit;
    filterOrbit.destruct();

    THEN("All plans pass (dead filter = no filter)") {
      CHECK(planMatchesFilters(f.plan1, state));
      CHECK(planMatchesFilters(f.plan2, state));
      CHECK(planMatchesFilters(f.plan3, state));
    }
  }
}
