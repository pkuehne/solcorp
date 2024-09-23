#include "gui.h"
#include "components.h"
#include "gui/site_window.h"

/// @brief System to draw the Site Window
/// @param iter Access to flecs
/// @param gui GUI related data
void systemDrawSiteWindow(flecs::iter &iter, size_t, GuiResource &gui) {
  auto world = iter.world();
  gui.site_window.draw(world);
}
