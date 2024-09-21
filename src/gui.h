#pragma once
#include "graphics.h"

void initialiseGUI(flecs::world &);
void systemUpdateUI(flecs::iter &, size_t, GuiResource &);
void systemRenderUI(const Renderer &);
void systemOpenLaunchWindow(flecs::entity entity, GuiResource &gui,
                            const OpenLaunchWindow &);
