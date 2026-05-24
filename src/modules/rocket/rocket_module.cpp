#include "rocket_module.h"
#include "active_launches_window.h"
#include "contracts_window.h"
#include "launch_actions.h"
#include "launch_window.h"
#include "modules/base/action.h"
#include "modules/base/assert.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "rocket_actions.h"

#include "modules/engine/engine.h"
#include "modules/engine/gui.h"
#include "modules/engine/helpers.h"
#include "modules/lua/lua.h"
#include "modules/site/helpers.h"
#include "spdlog/spdlog.h"
#include <flecs.h>
#include <flecs/addons/cpp/c_types.hpp>
#include <memory>
#include <modules/simulation/simulation.h>
#include <vector>

uint32_t LaunchPlan::max_id = 1;
uint32_t Rocket::max_id = 1;

/// @brief Module Constructor
/// Sets up all necessary components, GUIs and Systems
RocketModule::RocketModule(flecs::world &world) {

  world.import <BaseModule>();
  world.import <StatsModule>();

  registerEngineComponents(world);

  // Register components
  world.component<ContractFilterStatus>();
  world.component<ScheduleLaunchAction>("PlannedLaunch")
      .member("name", &ScheduleLaunchAction::name)
      .member("launchDay", &ScheduleLaunchAction::launchDay)
      .member("rocket", &ScheduleLaunchAction::rocket)
      .member("launchpad", &ScheduleLaunchAction::launchpad);
  world.component<Rocket>()
      .member("failure_rate", &Rocket::failure_rate)
      .member("cost", &Rocket::cost);

  world.component<RocketStateTransitionBlocked>().member(
      "reason", &RocketStateTransitionBlocked::reason);
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
  world.component<RocketTargetParent>();
  world.component<RocketCurrentState>().add(flecs::Exclusive);
  world.component<RocketTargetState>().add(flecs::Exclusive);
  world.component<LaunchPlanCurrentState>().add(flecs::Exclusive);
  world.component<LaunchPlanTargetState>().add(flecs::Exclusive);

  // Register Lua bindings
  register_component_lua<LaunchPlan>(
      world, "LaunchPlan", [](LuaFieldBuilder<LaunchPlan> &b) {
        b.field<&LaunchPlan::launch_date>("launch_date");
      });
  register_component_lua<Rocket>(
      world, "Rocket", [](LuaFieldBuilder<Rocket> &b) {
        b.nested<&Rocket::failure_rate>({"failure_rate"}, {"Stat"})
            .nested<&Rocket::cost>({"cost"}, {"Stat"});
      });
  register_component_lua<RocketCurrentState>(
      world, "RocketCurrentState",
      [](LuaFieldBuilder<RocketCurrentState> &) {});
  register_component_lua<RocketTargetState>(
      world, "RocketTargetState", [](LuaFieldBuilder<RocketTargetState> &) {});
  register_component_lua<LaunchPlanCurrentState>(
      world, "LaunchPlanCurrentState",
      [](LuaFieldBuilder<LaunchPlanCurrentState> &) {});
  register_component_lua<LaunchPlanTargetState>(
      world, "LaunchPlanTargetState",
      [](LuaFieldBuilder<LaunchPlanTargetState> &) {});
  register_component_lua<RocketStateTransitionBlocked>(
      world, "RocketStateTransitionBlocked",
      [](LuaFieldBuilder<RocketStateTransitionBlocked> &b) {
        b.field<&RocketStateTransitionBlocked::reason>("reason");
      });
  register_component_lua<Payload>(
      world, "Payload",
      [](LuaFieldBuilder<Payload> &b) { b.field<&Payload::mass>("mass"); });
  register_component_lua<CanLiftTo>(world, "CanLiftTo",
                                    [](LuaFieldBuilder<CanLiftTo> &b) {
                                      b.field<&CanLiftTo::max_mass>("max_mass");
                                    });
  register_component_lua<Contract>(
      world, "Contract", [](LuaFieldBuilder<Contract> &b) {
        b.field<&Contract::client>("client")
            .field<&Contract::description>("description")
            .field<&Contract::upfront_payment>("upfront_payment")
            .field<&Contract::completion_payment>("completion_payment")
            .field<&Contract::status>("status")
            .field<&Contract::failed>("failed");
      });
  register_enum_table_lua(world, "ContractStatus", [](LuaEnumBuilder &b) {
    b.value("Open", ContractStatus::Open)
        .value("Accepted", ContractStatus::Accepted)
        .value("Closed", ContractStatus::Closed);
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
  world.system("Create Rocket Build Notification Category")
      .kind(flecs::OnStart)
      .immediate()
      .run(systemCreateRocketBuildCategory);
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
  world.system<Rocket>("Rocket Complete State Transition Action")
      .immediate()
      .tick_source(sim.speed)
      .with<RocketTargetState>(flecs::Wildcard)
      .without<EffortRequired>()
      .without<DurationRequired>()
      .kind(UpdatePhase)
      .each(systemRocketCompleteAction);

  auto scope = world.set_scope(0);
  auto statesRoot = world.entity("States");
  auto rocketStates = world.entity("Rocket").child_of(statesRoot);
  world.entity("Invalid").child_of(rocketStates);
  world.entity("UnderConstruction").child_of(rocketStates);
  world.entity("Stored").child_of(rocketStates);
  world.entity("Moving").child_of(rocketStates);
  world.entity("Assigned").child_of(rocketStates);
  auto planStates = world.entity("LaunchPlan").child_of(statesRoot);
  world.entity("Scheduled").child_of(planStates);
  world.set_scope(scope);
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

  auto &company = world.get_mut<Company>();
  uint32_t total_payment = 0;

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
        instantiateNotification(
            world, "Launch Failure",
            fmt::format("{} was launched on {}, which failed."
                        " Contract {} is failed.",
                        payload.name().c_str(), rocketE.name().c_str(),
                        contractE.name().c_str()));
      } else {
        contract.failed = false;
        company.balance += static_cast<int64_t>(contract.completion_payment);
        total_payment += contract.completion_payment;
        instantiateNotification(
            world, "Payload Launched",
            fmt::format("{} was launched on {} to {}", payload.name().c_str(),
                        rocketE.name().c_str(),
                        plan.target_orbit.name().c_str()),
            world.lookup("NotificationCategories::Rocket Launch"));
      }
      contract.status = ContractStatus::Closed;
    }
  }

  auto launchpadE = planE.target<LaunchingFrom>();
  spdlog::debug("Removing plan: {} launch_date: {} today: {}", planE.id(),
                plan.launch_date, today);

  std::string notification;
  if (rocket_failure) {
    notification = std::format("{} failed - {} exploded on launch",
                               planE.name().c_str(), rocketE.name().c_str());
  } else {
    notification = std::format("{} launched {} successfully ({})",
                               planE.name().c_str(), rocketE.name().c_str(),
                               ("$" + format_locale(total_payment)).c_str());
  }
  notification +=
      fmt::format("\nOrbit:     {}", plan.target_orbit.name().c_str());
  notification += fmt::format("\nLaunchpad: {}", launchpadE.name().c_str());
  notification += fmt::format("\nRocket:    {}", rocketE.name().c_str());
  std::string payload_list;
  for (auto payload : payloads) {
    payload_list += "\n- " + std::string(payload.name().c_str());
  }
  notification += "\nPayloads:" + payload_list;

  instantiateBuildingNotification(world, launchpadE, notification);
  instantiateNotification(world, "Launch Complete", notification,
                          world.lookup("NotificationCategories::Rocket Launch"),
                          rocket_failure ? NotificationSeverity::Critical
                                         : NotificationSeverity::High);

  for (auto payload : payloads) {
    payload.destruct();
  }
  rocketE.destruct();
  planE.destruct();
}

void systemCreateRocketBuildCategory(flecs::iter &it) {
  auto world = it.world();
  createNotificationCategory(world, "Rocket Build");
  createNotificationCategory(world, "Rocket Launch");
  createNotificationCategory(world, "Rocket Move");
  createNotificationCategory(world, "Contracts");
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
  world.prefab("Rocket")
      .child_of(core_node)
      .add<Rocket>()
      .add<RocketCurrentState>(world.lookup("States::Rocket::Stored"));
}

void systemRocketCompleteAction(flecs::entity e, Rocket &) {
  auto world = e.world();

  std::unique_ptr<IAction> action;
  auto current_state = e.target<RocketCurrentState>();
  if (current_state == world.lookup("States::Rocket::UnderConstruction")) {
    action = std::make_unique<RocketCompleteBuildAction>(e);
  } else if (current_state == world.lookup("States::Rocket::Moving")) {
    action = std::make_unique<RocketCompleteMoveAction>(e);
  }

  if (!action) {
    spdlog::error("No completion action found for rocket {} in state: {}",
                  e.name().c_str(), current_state.name().c_str());
    return;
  }
  if (action->validate(world)) {
    action->execute(world);
  } else {
    action->block(world);
  }
}
