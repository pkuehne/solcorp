#include "rocket_launch.h"
#include "actions.h"
#include "active_launches_window.h"
#include "contracts_window.h"
#include "launch_window.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/lua/lua.h"
#include "modules/site/helpers.h"
#include "spdlog/spdlog.h"
#include <flecs.h>

u_int LaunchPlan::max_id = 1;
u_int Rocket::max_id = 1;

/// @brief Module Constructor
/// Sets up all necessary components, GUIs and Systems
RocketLaunchModule::RocketLaunchModule(flecs::world &world) {
  spdlog::debug("Loading RocketLaunchModule");

  world.import<BaseModule>();

  // Register components
  world.component<ContractFilterStatus>();
  world.component<ScheduleLaunchAction>("PlannedLaunch")
      .member("name", &ScheduleLaunchAction::name)
      .member("launchDay", &ScheduleLaunchAction::launchDay)
      .member("rocket", &ScheduleLaunchAction::rocket)
      .member("launchpad", &ScheduleLaunchAction::launchpad);
  world.component<Rocket>();
  world.component<Payload>();
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
  world.component<ContractPayload>();

  // Register relationships
  world.component<LaunchingWith>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingOn>().add(flecs::Exclusive).add(flecs::Symmetric);
  world.component<LaunchingFrom>().add(
      flecs::Symmetric); // Not Exclusive because each Launchpad can have
                         // multiple Plans assigned
  world.component<CanLiftTo>().add(flecs::Symmetric);

  // Register Lua bindings
  register_lua_user_type<LaunchPlan>(
      world, "LaunchPlan", [](sol::usertype<LaunchPlan> &userType) {
        userType["launch_date"] = &LaunchPlan::launch_date;
      });
  register_lua_user_type<Rocket>(world, "Rocket",
                                 [](sol::usertype<Rocket> &) {});
  register_lua_user_type<Payload>(world, "Payload",
                                  [](sol::usertype<Payload> &userType) {
                                    userType["mass"] = &Payload::mass;
                                  });
  register_lua_user_type<CanLiftTo>(
      world, "CanLiftTo", [](sol::usertype<CanLiftTo> &userType) {
        userType["max_mass"] = &CanLiftTo::max_mass;
      });
  register_lua_user_type<Contract>(
      world, "Contract", [](sol::usertype<Contract> &userType) {
        userType["client"] = &Contract::client;
        userType["description"] = &Contract::description;
        userType["upfront_payment"] = &Contract::upfront_payment;
        userType["completion_payment"] = &Contract::completion_payment;
        userType["status"] = &Contract::status;
        userType["failed"] = &Contract::failed;
      });
  register_lua_enum_table<ContractStatus>(
      world, "ContractStatus", [](sol::table &enumTable) {
        enumTable["Open"] = ContractStatus::Open;
        enumTable["Accepted"] = ContractStatus::Accepted;
        enumTable["Closed"] = ContractStatus::Closed;
      });
  register_lua_user_type<ContractPayload>(
      world, "ContractPayload", [](sol::usertype<ContractPayload> &) {});
  register_lua_user_type<ContractTargetOrbit>(
      world, "ContractTargetOrbit",
      [](sol::usertype<ContractTargetOrbit> &) {});

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
  u_int today = world.get<Game>().day;

  if (plan.launch_date > today) {
    return;
  }

  auto rocketE = planE.target<LaunchingOn>();
  if (rocketE.is_valid()) {
    spdlog::debug("Removing rocket: {}", rocketE.id());
    rocketE.destruct();
  }
  auto payloadE = planE.target<LaunchingWith>();
  if (payloadE.is_valid()) {
    spdlog::debug("Removing payload: {}", payloadE.id());

    payloadE.destruct();
  }
  auto launchpadE = planE.target<LaunchingFrom>();
  spdlog::debug("Removing plan: {} launch_date: {} today: {}", planE.id(),
                plan.launch_date, today);
  instantiateBuildingNotification(
      world, launchpadE, fmt::format("{} launched", planE.name().c_str()));
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
