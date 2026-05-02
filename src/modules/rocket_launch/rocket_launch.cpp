#include "rocket_launch.h"
#include "actions.h"
#include "active_launches_window.h"
#include "contracts_window.h"
#include "launch_window.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/engine/engine.h"
#include "modules/engine/gui.h"
#include "modules/engine/helpers.h"
#include "modules/lua/lua.h"
#include "modules/site/helpers.h"
#include "modules/site/site.h"
#include "spdlog/spdlog.h"
#include <flecs.h>
#include <modules/simulation/simulation.h>
#include <vector>

uint32_t LaunchPlan::max_id = 1;
uint32_t Rocket::max_id = 1;

/// @brief Module Constructor
/// Sets up all necessary components, GUIs and Systems
RocketLaunchModule::RocketLaunchModule(flecs::world &world) {
  spdlog::debug("Loading RocketLaunchModule");

  world.import <BaseModule>();
  world.import <StatsModule>();

  registerEngineComponents(world);

  // Register components
  world.component<Construction>()
      .member("effort_remaining", &Construction::effort_remaining)
      .member("effort_total", &Construction::effort_total);
  world.component<ContractFilterStatus>();
  world.component<ScheduleLaunchAction>("PlannedLaunch")
      .member("name", &ScheduleLaunchAction::name)
      .member("launchDay", &ScheduleLaunchAction::launchDay)
      .member("rocket", &ScheduleLaunchAction::rocket)
      .member("launchpad", &ScheduleLaunchAction::launchpad);
  world.component<Rocket>()
      .member("failure_rate", &Rocket::failure_rate)
      .member("cost", &Rocket::cost);
  world.component<Payload>().member("mass", &Payload::mass);
  world.component<CanLiftTo>().member("max_mass", &CanLiftTo::max_mass);
  world.component<LaunchPlan>();
  world.component<LaunchWindow>().member("draftPlan", &LaunchWindow::draftPlan);
  world.component<ActiveLaunchesWindow>()
      .member("filterSite", &ActiveLaunchesWindow::filterSite)
      .member("filterPad", &ActiveLaunchesWindow::filterPad)
      .member("filterOrbit", &ActiveLaunchesWindow::filterOrbit)
      .member("pendingCancel", &ActiveLaunchesWindow::pendingCancel);
  world.component<ContractsWindow>()
      .member("statusFilter", &ContractsWindow::statusFilter)
      .member("showCompleted", &ContractsWindow::showCompleted)
      .member("pendingDelete", &ContractsWindow::pendingDelete);
  world.component<ContractTargetOrbit>();
  world.component<ContractStatus>();
  world.component<Contract>()
      .member("client", &Contract::client)
      .member("description", &Contract::description)
      .member("upfront_payment", &Contract::upfront_payment)
      .member("completion_payment", &Contract::completion_payment)
      .member("status", &Contract::status)
      .member("failed", &Contract::failed);
  world.component<ContractPayload>().add(flecs::Symmetric);

  // Register relationships
  world.component<LaunchingWith>().add(flecs::Symmetric);
  world.component<LaunchingOn>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingFrom>().add(
      flecs::Symmetric); // Not Exclusive because each Launchpad can have
                         // multiple Plans assigned
  world.component<CanLiftTo>().add(flecs::Symmetric);

  // Register Lua bindings
  register_component_lua<LaunchPlan>(world, "LaunchPlan", [](lua_State *L, int mt) {
    lua_register_field<&LaunchPlan::launch_date>(L, mt, "launch_date");
  });
  register_component_lua<Rocket>(world, "Rocket", [](lua_State *L, int mt) {
    lua_getfield(L, mt, "__getters");
    lua_pushcfunction(L, [](lua_State *Lx) -> int {
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      lua_push_component(Lx, &static_cast<Rocket *>(ud->ptr)->failure_rate,
                         "solcorp.Stat", false, nullptr);
      return 1;
    });
    lua_setfield(L, -2, "failure_rate");
    lua_pushcfunction(L, [](lua_State *Lx) -> int {
      auto *ud = static_cast<ComponentUD *>(lua_touserdata(Lx, 1));
      lua_push_component(Lx, &static_cast<Rocket *>(ud->ptr)->cost,
                         "solcorp.Stat", false, nullptr);
      return 1;
    });
    lua_setfield(L, -2, "cost");
    lua_pop(L, 1);
  });
  register_component_lua<Payload>(world, "Payload", [](lua_State *L, int mt) {
    lua_register_field<&Payload::mass>(L, mt, "mass");
  });
  register_component_lua<CanLiftTo>(world, "CanLiftTo", [](lua_State *L, int mt) {
    lua_register_field<&CanLiftTo::max_mass>(L, mt, "max_mass");
  });
  register_component_lua<Contract>(world, "Contract", [](lua_State *L, int mt) {
    lua_register_field<&Contract::client>(L, mt, "client");
    lua_register_field<&Contract::description>(L, mt, "description");
    lua_register_field<&Contract::upfront_payment>(L, mt, "upfront_payment");
    lua_register_field<&Contract::completion_payment>(L, mt, "completion_payment");
    lua_register_field<&Contract::status>(L, mt, "status");
    lua_register_field<&Contract::failed>(L, mt, "failed");
  });
  register_enum_table_lua(world, "ContractStatus", [](lua_State *L, int tbl) {
    lua_pushinteger(L, static_cast<lua_Integer>(ContractStatus::Open));
    lua_setfield(L, tbl, "Open");
    lua_pushinteger(L, static_cast<lua_Integer>(ContractStatus::Accepted));
    lua_setfield(L, tbl, "Accepted");
    lua_pushinteger(L, static_cast<lua_Integer>(ContractStatus::Closed));
    lua_setfield(L, tbl, "Closed");
  });
  register_component_lua<ContractPayload>(world, "ContractPayload");
  register_component_lua<ContractTargetOrbit>(world, "ContractTargetOrbit");

  // Register systems
  world.system("Create Rocket Prefabs")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemCreateRocketPrefabs);
  world.system("Create Contract Node")
      .kind(flecs::OnStart)
      .immediate()
      .run([](flecs::iter &it) {
        auto world = it.world();
        world.entity("Contracts");
      });
  auto sim = world.get<Simulation>();
  world.system<LaunchPlan>("Launch Rocket")
      .tick_source(sim.speed)
      .kind(UpdatePhase)
      .each(systemLaunchRocket);
  world.system("Rocket Launch Create Windows")
      .kind(flecs::OnStart)
      .immediate()
      .run([](flecs::iter &it) {
        auto world = it.world();
        registerWindow("Mission Plan", drawLaunchWindow, world)
            .set<LaunchWindow>({});
        registerWindow("Active Launches", drawActiveLaunchesWindow, world)
            .set<ActiveLaunchesWindow>({});
        registerWindow("Contracts Window", drawContractsWindow, world)
            .set<ContractsWindow>({});
      });
}

/// @brief Process LaunchPlans that are due
/// Ensures that the rocket is destroyed after being launched
/// Also removes the launchplan and clears all relationships
/// @param planE The plan's entity
/// @param plan The plan's component
void systemLaunchRocket(flecs::entity planE, LaunchPlan &plan) {
  auto world = planE.world();
  uint32_t today = world.get<Game>().day;

  if (plan.launch_date > today) {
    return;
  }

  auto rocketE = planE.target<LaunchingOn>();

  bool rocket_failure = roll_random(rocketE.get<Rocket>().failure_rate.value());
  spdlog::info("Rocket Launch failure {} based on chance: {}", rocket_failure,
               rocketE.get<Rocket>().failure_rate.value());
  std::vector<flecs::entity> payloads;
  planE.each<LaunchingWith>([&](flecs::entity payload) {
    if (payload.is_valid() && payload.has<Payload>()) {
      payloads.push_back(payload);
    }
  });

  Company &company = world.get_mut<Company>();
  double total_payment = 0.0;

  for (auto payload : payloads) {
    if (payload.is_valid() && payload.has<Payload>()) {
      spdlog::debug("Removing payload: {}", payload.name().c_str());
      auto contractE = payload.target<ContractPayload>();
      SC_ASSERT(contractE.is_valid() && contractE.has<Contract>(),
                "Payload {} has ContractPayload relationship to invalid or "
                "non-contract entity");

      auto &contract = contractE.get_mut<Contract>();
      if (contractE.target<ContractTargetOrbit>() != plan.target_orbit) {
        spdlog::info(
            "Contract {} failed because payload {} was launched to wrong orbit",
            contractE.name().c_str(), payload.name().c_str());
        contract.failed = true;
      } else if (rocket_failure) {
        spdlog::info(
            "Contract {} failed because payload {} was launched on a rocket "
            "that failed",
            contractE.name().c_str(), payload.name().c_str());
        contract.failed = true;
      } else {
        contract.failed = false;
        company.balance += contract.completion_payment;
        total_payment += contract.completion_payment;
      }
      contract.status = ContractStatus::Closed;

      payload.destruct();
      // TODO: We should probably also instantiate a notification for the
      // payload being launched, but that requires some refactoring of the
      // notification system to allow for multiple notifications in the same
      // tick
    }
  }

  auto launchpadE = planE.target<LaunchingFrom>();
  spdlog::debug("Removing plan: {} launch_date: {} today: {}", planE.id(),
                plan.launch_date, today);
  std::string notification =
      rocket_failure ? std::format("{} failed - {} exploded on launch",
                                   planE.name().c_str(), rocketE.name().c_str())
                     : std::format("{} launched successfully (${:.0f})",
                                   planE.name().c_str(), total_payment);
  instantiateBuildingNotification(world, launchpadE, notification);

  rocketE.destruct();
  planE.destruct();
}

void systemCreateRocketPrefabs(flecs::iter &it) {
  const flecs::world &world = it.world();

  spdlog::debug("Creating Rocket Prefabs");
  auto prefabs_node = world.lookup("Prefabs");
  if (!prefabs_node.is_valid()) {
    prefabs_node = world.entity("Prefabs");
  }
  auto core_node = world.lookup("Prefabs::Core");
  if (!core_node.is_valid()) {
    core_node = world.entity("Core").child_of(world.entity("Prefabs"));
  }
  auto rocket_node = world.lookup("Prefabs::Rockets");
  if (!rocket_node.is_valid()) {
    rocket_node = world.entity("Rockets").child_of(prefabs_node);
  }

  // Base Rocket Prefab
  world.prefab("Rocket").child_of(core_node).add<Rocket>();
}
