#include "window_module.h"
#include "SDL_keycode.h"
#include "main_menu.h"
#include "modules/base/base.h"
#include "modules/engine/gui.h"
#include "modules/engine/input.h"
#include "notification_window.h"
#include <flecs.h>
#include <modules/simulation/simulation.h>

WindowModule::WindowModule(flecs::world &world) {

  // Register components
  world.component<MainMenuBar>();
  world.component<NotificationWindow>()
      .member("severity_filter", &NotificationWindow::severity_filter)
      .member("category_filter", &NotificationWindow::category_filter)
      .member("unread_only", &NotificationWindow::unread_only);

  // Register window
  world.entity("MainMenuBar").add<MainMenuBar>();

  // Must be registered in OnStart (outside module scope) so the entity is
  // parented to the root "Windows" node, not the WindowModule entity.
  world.system("Register Notification Window")
      .kind(flecs::OnStart)
      .run([](flecs::iter &it) {
        auto w = it.world();
        registerWindow("Notifications", drawNotificationWindow, w)
            .set<NotificationWindow>({});
      });

  // Register Systems
  world.system<const Simulation, const Game, MainMenuBar>("Draw MainMenu")
      .kind(GuiPhase)
      .each(systemDrawMainMenu);
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

void systemTickDurationRequired(flecs::entity, DurationRequired &duration) {
  if (duration.remaining > 0) {
    duration.remaining--;
  }
}
