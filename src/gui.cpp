#include "gui.h"
#include <TGUI/AllWidgets.hpp>
#include <TGUI/Backend/SDL-Renderer.hpp>
#include <TGUI/Core.hpp>
#include <spdlog/spdlog.h>

/// @brief Creates and initialises the GUI system
/// @param world
void initialiseGUI(flecs::world &world) {
  spdlog::info("Creating GUI");

  auto r = world.get_mut<Renderer>();

  auto popLaunchSite = tgui::ChildWindow::create();
  // child->setRenderer(theme.getRenderer("ChildWindow"));
  popLaunchSite->setClientSize({250, 120});
  popLaunchSite->setPosition("50%", "50%");
  popLaunchSite->setOrigin(0.5f, 0.5f);
  popLaunchSite->setTitle("Launch Site");
  popLaunchSite->setVisible(false);
  popLaunchSite->onClosing([=](bool *abort) {
    *abort = true; // Prevents the window from being closed, onClose won't be
                   // triggered
    popLaunchSite->setVisible(false);
  });
  r->gui->add(popLaunchSite, "popLaunchSite");

  auto lblName = tgui::Label::create();
  lblName->setText("Name: ");
  popLaunchSite->add(lblName, "lblName");

  auto panel = tgui::Panel::create();
  panel->setPosition(0, 0);
  // panel->getRenderer()->setBackgroundColor(tgui::Color::Green);
  r->gui->add(panel);

  auto lblDay = tgui::Label::create();
  // label->setRenderer(theme.getRenderer("Label"));
  lblDay->setText("Day: 0");
  lblDay->setPosition(0, 0);
  // label->setTextSize(36);
  panel->add(lblDay, "lblDay");
  panel->setSize(r->gui->getView().getSize().x, lblDay->getSize().y);
}

/// @brief System to update the UI from the current game state
/// @param it
/// @param game The game resource
/// @param r The renderer
void systemUpdateUI(flecs::iter &it, GameResource *game, Renderer *r) {
  auto world = it.world();
  auto gui = r->gui;

  auto label = gui->get<tgui::Label>("lblDay");
  if (label == nullptr) {
    spdlog::critical("Failed to find widget: {}", "");
    world.quit();
    return;
  }
  label->setText("Day: " + std::to_string(game->day));
}

/// @brief System to send render instructions for the UI
/// @param it
/// @param renderer The Render Component Singleton
void systemRenderUI(flecs::iter &, const Renderer *r) { r->gui->draw(); }
