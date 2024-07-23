#pragma once
#include "graphics.h"

void initialiseGUI(flecs::world &);
void systemUpdateUI(flecs::iter &, GuiResource *);
void systemRenderUI(flecs::iter &, const Renderer *);
