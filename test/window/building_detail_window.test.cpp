#include "modules/window/building_detail_window.h"
#include "modules/base/base.h"
#include "modules/rocket/rocket_module.h"
#include "modules/simulation/simulation.h"
#include "modules/site/site.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <flecs.h>
#include <vector>

SCENARIO("getEntityEffortRequired", "[window]") {
  flecs::world world;
  world.component<EffortRequired>();

  GIVEN("An entity with no EffortRequired component") {
    auto entity = world.entity();
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 1.0 (nothing to wait for)") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(1.0f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired fully remaining") {
    auto entity =
        world.entity().set<EffortRequired>({.remaining = 100, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 0.0 (no progress)") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.0f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired fully complete") {
    auto entity =
        world.entity().set<EffortRequired>({.remaining = 0, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 1.0 (fully done)") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(1.0f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired halfway complete") {
    auto entity =
        world.entity().set<EffortRequired>({.remaining = 50, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 0.5") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.5f, 0.001f));
      }
    }
  }

  GIVEN("An entity with EffortRequired at one tick remaining") {
    auto entity =
        world.entity().set<EffortRequired>({.remaining = 1, .total = 100});
    WHEN("getEntityEffortRequired is called") {
      float result = getEntityEffortRequired(entity);
      THEN("it returns 0.99") {
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.99f, 0.001f));
      }
    }
  }
}

SCENARIO("activeLaunchPlansForPad", "[window]") {
  flecs::world world;
  world.import <SimulationModule>();
  world.import <SiteModule>();
  world.import <RocketModule>();

  GIVEN("A launchpad with plans in various states and a plan on another pad") {
    auto pad = world.entity("Pad").add<Launchpad>();
    auto otherPad = world.entity("Other Pad").add<Launchpad>();

    auto makePlan = [&](const char *name, flecs::entity padE,
                        const char *state) {
      auto plan = world.entity(name).set<LaunchPlan>({});
      plan.add<LaunchingFrom>(padE);
      plan.add<LaunchPlanCurrentState>(world.lookup(state));
      return plan;
    };

    auto scheduled =
        makePlan("Scheduled", pad, "States::LaunchPlan::Scheduled");
    auto onPad = makePlan("OnPad", pad, "States::LaunchPlan::OnPad");
    auto launched = makePlan("Launched", pad, "States::LaunchPlan::Launched");
    auto cancelled =
        makePlan("Cancelled", pad, "States::LaunchPlan::Cancelled");
    auto otherPadPlan =
        makePlan("OtherPadPlan", otherPad, "States::LaunchPlan::Scheduled");

    WHEN("Querying active plans for the pad") {
      std::vector<flecs::entity> results;
      activeLaunchPlansForPad(pad).each(
          [&](flecs::entity planE, LaunchPlan &) {
            results.push_back(planE);
          });

      THEN("Only the non-terminal plans on that pad are returned") {
        CHECK(results.size() == 2);
        CHECK(std::ranges::find(results, scheduled) != results.end());
        CHECK(std::ranges::find(results, onPad) != results.end());
      }
      THEN("Plans in terminal states are excluded") {
        CHECK(std::ranges::find(results, launched) == results.end());
        CHECK(std::ranges::find(results, cancelled) == results.end());
      }
      THEN("Plans on other launchpads are excluded") {
        CHECK(std::ranges::find(results, otherPadPlan) == results.end());
      }
    }
  }
}
