#include "rocket_module.h"
#include "active_launches_window.h"
#include "launch_actions.h"
#include "modules/base/action.h"
#include "modules/base/base.h"
#include "modules/base/notification.h"
#include "modules/engine/engine.h"
#include "modules/engine/gui.h"
#include "modules/lua/lua.h"
#include "rocket_actions.h"
#include "spdlog/spdlog.h"
#include <flecs.h>
#include <flecs/addons/cpp/c_types.hpp>
#include <memory>
#include <modules/simulation/simulation.h>

uint32_t LaunchPlan::max_id = 1;
uint32_t Rocket::max_id = 1;
uint32_t Contract::max_id = 1;

/// @brief Module Constructor
/// Sets up all necessary components, GUIs and Systems
RocketModule::RocketModule(flecs::world &world) {

  world.import <BaseModule>();
  world.import <StatsModule>();

  registerEngineComponents(world);

  // Register components
  world.component<LaunchScheduleAction>("PlannedLaunch")
      .member("name", &LaunchScheduleAction::name)
      .member("launchDay", &LaunchScheduleAction::launchDay)
      .member("rocket", &LaunchScheduleAction::rocket)
      .member("launchpad", &LaunchScheduleAction::launchpad);
  world.component<Rocket>()
      .member("failure_rate", &Rocket::failure_rate)
      .member("cost", &Rocket::cost)
      .member("rollout_days", &Rocket::rollout_days)
      .member("move_days", &Rocket::move_days);
  registerStatDef(world,
                  {.id = "failure-rate",
                   .display = "Failure Rate",
                   .description = "Likelihood the rocket will fail on take-off",
                   .higher_is_better = false,
                   .format = Stat::Format::Percentage});
  registerStatDef(world, {.id = "cost",
                          .display = "Cost",
                          .description = "Cost to build this rocket",
                          .higher_is_better = false,
                          .format = Stat::Format::Currency});
  registerStatDef(
      world,
      {.id = "rollout-days",
       .display = "Rollout Duration",
       .description = "Days to move the rocket from storage to the launchpad",
       .higher_is_better = false});
  registerStatDef(
      world, {.id = "move-days",
              .display = "Move Duration",
              .description = "Days to move the rocket from storage to storage",
              .higher_is_better = false});

  world.component<RocketStateTransitionBlocked>().member(
      "reason", &RocketStateTransitionBlocked::reason);
  world.component<Payload>().member("mass", &Payload::mass);
  world.component<CanLiftTo>().member("max_mass", &CanLiftTo::max_mass);
  world.component<LaunchPlan>();
  world.component<ActiveLaunchesWindow>()
      .member("filterSite", &ActiveLaunchesWindow::filterSite)
      .member("filterPad", &ActiveLaunchesWindow::filterPad)
      .member("filterOrbit", &ActiveLaunchesWindow::filterOrbit)
      .member("pendingCancel", &ActiveLaunchesWindow::pendingCancel)
      .member("showCompleted", &ActiveLaunchesWindow::showCompleted);
  world.component<ContractTargetOrbit>();
  world.component<Contract>()
      .member("name", &Contract::name)
      .member("client", &Contract::client)
      .member("description", &Contract::description)
      .member("upfront_payment", &Contract::upfront_payment)
      .member("completion_payment", &Contract::completion_payment)
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
  world.component<ContractCurrentState>().add(flecs::Exclusive);
  world.component<ContractTargetState>().add(flecs::Exclusive);

  // Register Lua bindings
  register_component_lua<LaunchPlan>(
      world, "LaunchPlan", [](LuaFieldBuilder<LaunchPlan> &b) {
        b.field<&LaunchPlan::launch_date>("launch_date");
      });
  register_component_lua<Rocket>(
      world, "Rocket", [](LuaFieldBuilder<Rocket> &b) {
        b.nested<&Rocket::failure_rate>({"failure_rate"}, {"Stat"})
            .nested<&Rocket::cost>({"cost"}, {"Stat"})
            .nested<&Rocket::rollout_days>({"rollout_days"}, {"Stat"})
            .nested<&Rocket::move_days>({"move_days"}, {"Stat"});
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
  register_component_lua<ContractCurrentState>(
      world, "ContractCurrentState",
      [](LuaFieldBuilder<ContractCurrentState> &) {});
  register_component_lua<ContractTargetState>(
      world, "ContractTargetState",
      [](LuaFieldBuilder<ContractTargetState> &) {});
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
        b.field<&Contract::name>("name")
            .field<&Contract::client>("client")
            .field<&Contract::description>("description")
            .field<&Contract::upfront_payment>("upfront_payment")
            .field<&Contract::completion_payment>("completion_payment")
            .field<&Contract::failed>("failed");
      });
  register_component_lua<ContractPayload>(world, "ContractPayload");
  register_component_lua<ContractTargetOrbit>(world, "ContractTargetOrbit");

  // Create state entities before system registration so they can be used in
  // system filters
  // TODO(#201): We should have a guard-style class here that stores the scope,
  // sets it to zero and restores it on destruction/close(). This should be a
  // re-usable class as we do this in multiple places
  auto scope = world.set_scope(0);
  auto statesRoot = world.entity("States");
  auto rocketStates = world.entity("Rocket").child_of(statesRoot);
  world.entity("Invalid").child_of(rocketStates).add<StateIsTerminal>();
  world.entity("UnderConstruction")
      .child_of(rocketStates)
      .set<Label>({"Under Construction"});
  world.entity("Stored").child_of(rocketStates).set<Label>({"In Storage"});
  world.entity("Moving").child_of(rocketStates).set<Label>({"Moving"});
  world.entity("Assigned").child_of(rocketStates).set<Label>({"Assigned"});
  world.entity("Launched")
      .child_of(rocketStates)
      .set<Label>({"Launched"})
      .add<StateIsTerminal>();
  auto planStates = world.entity("LaunchPlan").child_of(statesRoot);
  auto scheduledState = world.entity("Scheduled").child_of(planStates);
  auto rollingOutState = world.entity("RollingOut").child_of(planStates);
  auto onPadState = world.entity("OnPad").child_of(planStates);
  world.entity("Launched").child_of(planStates).add<StateIsTerminal>();
  world.entity("Cancelled").child_of(planStates).add<StateIsTerminal>();
  auto contractStates = world.entity("Contract").child_of(statesRoot);
  world.entity("Open").child_of(contractStates);
  world.entity("Accepted").child_of(contractStates);
  world.entity("Closed").child_of(contractStates).add<StateIsTerminal>();
  world.set_scope(scope);

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
  world.system("Rocket Launch Create Windows")
      .kind(flecs::OnStart)
      .immediate()
      .run([](flecs::iter &it) {
        auto world = it.world();
        registerWindow("Active Launches", drawActiveLaunchesWindow, world)
            .set<ActiveLaunchesWindow>({});
      });
  world.system<Rocket>("Rocket Complete State Transition Action")
      .immediate()
      .tick_source(sim.speed)
      .with<RocketTargetState>(flecs::Wildcard)
      .without<EffortRequired>()
      .without<DurationRequired>()
      .kind(UpdatePhase)
      .each(systemRocketCompleteAction);

  world.system<LaunchPlan>("Auto Initiate Rollout")
      .immediate()
      .tick_source(sim.speed)
      .with<LaunchPlanCurrentState>(scheduledState)
      .kind(UpdatePhase)
      .each(systemAutoInitiateRollout);

  world.system<LaunchPlan>("Auto Complete Rollout")
      .immediate()
      .tick_source(sim.speed)
      .with<LaunchPlanCurrentState>(rollingOutState)
      .kind(UpdatePhase)
      .each(systemAutoCompleteRollout);

  world.system<LaunchPlan>("Auto Go for Launch")
      .immediate()
      .tick_source(sim.speed)
      .with<LaunchPlanCurrentState>(onPadState)
      .without<DurationRequired>()
      .kind(UpdatePhase)
      .each(systemAutoGoForLaunch);
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

void systemAutoInitiateRollout(flecs::entity plan, LaunchPlan &planData) {
  auto world = plan.world();
  uint32_t today = world.get<Game>().day;
  if (today < planData.rollout_date) {
    return;
  }
  LaunchInitiateRolloutAction action{plan};
  if (action.validate(world)) {
    action.execute(world);
  }
}

void systemAutoCompleteRollout(flecs::entity plan, LaunchPlan &) {
  auto world = plan.world();
  LaunchCompleteRolloutAction action{plan};
  if (action.validate(world)) {
    action.execute(world);
  }
}

void systemAutoGoForLaunch(flecs::entity plan, LaunchPlan &) {
  auto world = plan.world();
  LaunchGoAction action{plan};
  if (action.validate(world)) {
    action.execute(world);
  }
}
