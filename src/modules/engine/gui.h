#pragma once

#include <flecs.h>
#include <functional>
#include <string>

struct Window {
  bool open = false;
  std::string title;
  std::function<void(flecs::entity)> content_renderer;
};

flecs::entity showWindow(flecs::world &world, const std::string &name);
flecs::entity hideWindow(flecs::world &world, const std::string &name);
flecs::entity
registerWindow(std::string name,
               std::function<void(flecs::entity)> content_renderer,
               flecs::world &world);

void registerGui(flecs::world &);
