#include "window_module.h"
#include "SDL_keycode.h"
#include "contract_detail_window.h"
#include "contracts_window.h"
#include "launch_detail_window.h"
#include "launch_window.h"
#include "main_menu.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/engine/input.h"
#include "modules/window/active_launches_window.h"
#include "modules/window/celestial_browser_window.h"
#include "notification_window.h"
#include "rocket_detail_window.h"
#include "rocket_list_window.h"
#include "toolbar.h"
#include <flecs.h>
#include <modules/simulation/simulation.h>

WindowModule::WindowModule(flecs::world &world) {

  // Register components
  world.component<MainMenuBar>();
  world.component<Toolbar>();
  world.component<ContractListWindow>()
      .member("stateFilter", &ContractListWindow::stateFilter)
      .member("hideTerminal", &ContractListWindow::hideTerminal);
  world.component<ContractDetailWindow>();
  world.component<RocketDetailWindow>();
  world.component<RocketListWindow>();
  world.component<CelestialBrowser>().member("selected_body",
                                             &CelestialBrowser::selected_body);
  world.component<ActiveLaunchesWindow>()
      .member("filterSite", &ActiveLaunchesWindow::filterSite)
      .member("filterPad", &ActiveLaunchesWindow::filterPad)
      .member("filterOrbit", &ActiveLaunchesWindow::filterOrbit)
      .member("pendingCancel", &ActiveLaunchesWindow::pendingCancel)
      .member("showCompleted", &ActiveLaunchesWindow::showCompleted);
  // Registered without the draftPlan reflection member: its type
  // (LaunchScheduleAction) is a rocket-domain action registered later in
  // RocketModule, which is imported after WindowModule.
  world.component<LaunchWindow>();
  world.component<LaunchDetailWindow>();
  world.component<NotificationWindow>()
      .member("severity_filter", &NotificationWindow::severity_filter)
      .member("category_filter", &NotificationWindow::category_filter)
      .member("unread_only", &NotificationWindow::unread_only);

  // Register window
  world.entity("MainMenuBar").add<MainMenuBar>();
  world.entity("Toolbar").add<Toolbar>();

  // Must be registered in OnStart (outside module scope) so the entity is
  // parented to the root "Windows" node, not the WindowModule entity.
  world.system("Register Windows")
      .kind(flecs::OnStart)
      .run([](flecs::iter &it) {
        auto w = it.world();
        registerWindow("Notifications", drawNotificationWindow, w)
            .set<NotificationWindow>({});
        registerWindow("Contract Detail", drawContractDetailWindow, w)
            .set<ContractDetailWindow>({});
        registerWindow("Contract List", drawContractListWindow, w)
            .set<ContractListWindow>({});
        registerWindow("Rocket Detail", drawRocketDetailWindow, w)
            .set<RocketDetailWindow>({});
        registerWindow("Rocket List", drawRocketListWindow, w)
            .set<RocketListWindow>({});
        registerWindow("Mission Plan", drawLaunchWindow, w)
            .set<LaunchWindow>({});
        registerWindow("Launch Detail", drawLaunchDetailWindow, w)
            .set<LaunchDetailWindow>({});
        registerWindow("Active Launches", drawActiveLaunchesWindow, w)
            .set<ActiveLaunchesWindow>({});
        registerWindow("Celestial Browser", drawCelestialBrowser, w)
            .set<CelestialBrowser>({});
      });

  // Register Systems
  world.system<const Simulation, const Game, MainMenuBar>("Draw MainMenu")
      .kind(GuiPhase)
      .each(systemDrawMainMenu);
  world.system<Toolbar>("Draw Toolbar").kind(GuiPhase).each(systemDrawToolbar);
  world.system<Simulation, const KeyDown>("Toggle Play/Pause")
      .kind(ValidatePhase)
      .each(systemToggle);
  auto sim = world.get<Simulation>();
  world.system<DurationRequired>("Tick DurationRequired")
      .kind(UpdatePhase)
      .tick_source(sim.speed)
      .each(systemTickDurationRequired);
}

void systemToggle(flecs::iter &it, size_t, Simulation &sim,
                  const KeyDown event) {
  if (event.key == SDLK_SPACE) {
    auto simTimer = it.world().timer(sim.speed.id());
    simTimer.get<flecs::Timer>().active ? simTimer.stop() : simTimer.start();
  }
}

void systemTickDurationRequired(flecs::entity e, DurationRequired &duration) {
  if (duration.remaining > 0) {
    duration.remaining--;
  }
  if (duration.remaining == 0) {
    e.remove<DurationRequired>();
  }
}
