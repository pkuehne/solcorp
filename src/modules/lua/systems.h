#pragma once

#include "modules/lua/lua.h"
#include <flecs.h>

void mod_on_start(flecs::entity, Mod &);
void mod_on_frame(flecs::entity, Mod &);
void mod_on_update(flecs::entity, Mod &);
