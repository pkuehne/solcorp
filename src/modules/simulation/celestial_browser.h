#include <flecs.h>
#include <string>

struct Window {
  bool open = false;
};

void showWindow(flecs::world &world, const std::string &name);
void hideWindow(flecs::world &world, const std::string &name);

struct CelestialBrowser {
  flecs::entity selected_body;
};
void systemDrawCelestialBrowser(flecs::entity winE, CelestialBrowser &win);
