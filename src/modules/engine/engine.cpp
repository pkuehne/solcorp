#include "engine.h"
#include "gui.h"
#include "input.h"
#include "modules/engine/movement.h"
#include "render.h"

EngineModule::EngineModule(flecs::world &world) {
  initialiseGraphics(world);

  registerRender(world);
  registerInput(world);
  registerGui(world);
  registerMovement(world);
}
