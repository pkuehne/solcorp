#pragma once
#include "graphics.h"

void initialiseGUI(flecs::world &);

void systemGuiNewFrame(flecs::iter &);
void systemDrawSiteWindow(flecs::iter &, size_t, GuiResource &);
void systemGuiEndFrame(flecs::iter &);

void systemRenderGUI(const Renderer &);
