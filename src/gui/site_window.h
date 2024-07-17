#pragma once

#include <flecs.h>

class SiteWindow {
public:
  void draw(flecs::world &world);

  bool visible = false;
};
