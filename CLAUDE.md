# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SolCorp is a C++20 space simulation game built on an **Entity Component System (ECS)** architecture using Flecs v4.1.2. The project features modular game systems, ImGui UI, Lua scripting for mods, and SDL2 graphics.

## Build Commands

The project uses CMake with Ninja and provides a Justfile for convenience:

```bash
# Initial setup (first time only)
just configure

# Build
just build
# or: cd build && ninja

# Run tests
just test
# or: cd build && ninja unit_tests

# Run the game
just run
# or: cd build && ninja run

# Release build
cmake --build build -c Release
```

### Nix Development Environment

The project uses a Nix flake for reproducible builds. Enter the dev shell with:
```bash
nix develop
```

This provides SDL2, lua5.4, imgui, catch2, gcc, gdb, and clangd.

## Architecture

### ECS Module System

The game is structured as **9 independent modules** in [src/modules/](src/modules/):
- **base**: Defines shared phases and global entities
- **engine**: Rendering (SDL2), input, movement, camera
- **lua**: Lua/C++ integration, component exposure, mod loading
- **simulation**: Celestial mechanics, orbital calculations
- **site**: Construction sites and facilities
- **staff**: Crew and personnel management
- **stats**: Statistics tracking
- **rocket_launch**: Launch operations and contracts
- **main**: Main menu UI

Each module is a struct with a constructor taking `flecs::world&` that registers components, systems, and phases.

### Execution Model

Systems execute in **11 defined phases** during the game loop:
1. PostStartPhase (initialization)
2. PreFramePhase
3. UpdatePhase (game logic)
4. PhysicsPhase
5. GuiPhase (ImGui rendering)
6. RenderPhase (SDL2 rendering)
7. PostFramePhase
... and more

The main loop in [src/main.cpp](src/main.cpp) runs at 60 FPS via `world.app().enable_stats().enable_rest().run()`.

### Module Registration Pattern

Modules are imported in [src/main.cpp](src/main.cpp) in a specific order:

```cpp
world.import<BaseModule>();      // Must be first (defines phases)
world.import<LuaModule>();        // Early (loads config)
world.import<EngineModule>();     // Core systems
world.import<StatsModule>();
world.import<SimulationModule>();
world.import<MainMenuModule>();
world.import<SiteModule>();
world.import<RocketLaunchModule>();
world.import<StaffModule>();
```

### System Registration

Systems are registered in module constructors using this pattern:

```cpp
world.system<ComponentType>("system_name")
  .kind(UpdatePhase)  // or GuiPhase, RenderPhase, etc.
  .iter(systemImplementationFunction);
```

### Component Definition Pattern

1. Define struct in module header (e.g., [src/modules/simulation/simulation.h](src/modules/simulation/simulation.h))
2. Add to [src/CMakeLists.txt](src/CMakeLists.txt) source lists
3. If Lua-exposed: Call `register_lua_user_type<ComponentType>()` in [src/modules/lua/lua.cpp](src/modules/lua/lua.cpp)

## Lua Integration

### Lua Mod System

Mods live in [mods/](mods/) directory. The core mod is [mods/core/init.lua](mods/core/init.lua).

Lua scripts expose handlers:
```lua
solcorp.script.handlers.on_init = function() end    -- Called once at startup
solcorp.script.handlers.on_start = function() end   -- Called after init
solcorp.script.handlers.on_update = function() end  -- Called per game update
solcorp.script.handlers.on_frame = function() end   -- Called per frame
```

### Lua Component Access

Components exposed to Lua use sol2 bindings via `register_lua_user_type<T>()`. This creates:
```lua
local pos = entity:getPosition()      -- getter
entity:setPosition(pos)               -- setter
if entity:hasPosition() then ... end  -- hasher
entity:removePosition()               -- remover
```

### Lua Helper Functions

The `solcorp.helpers` namespace provides convenience functions for common operations:
- `create_site(name, width, height, active)` - Create construction site
- `create_building(name, prefab, x, y, site)` - Place building at site
- `create_building_prefab(name)` - Define building type
- `create_rocket_prefab(name)` - Define rocket type
- `create_contract(...)` - Create launch contract
- `create_texture(name, path)` - Load texture from mods directory
- `clip_sprite_from_texture(...)` - Create sprite from texture atlas

See [mods/core/init.lua](mods/core/init.lua) for usage examples.

## Testing

Tests use Catch2 v3.6.0 and are in [test/](test/):
- `simulation.test.cpp` - Orbital mechanics tests
- `site.test.cpp` - Construction/site tests
- `rocket_launch_tests.cpp` - Launch system tests
- `active_launches_tests.cpp` - Active launches filter logic tests
- `stats.test.cpp` - Statistics tests
- etc.

Tests link against `solcorplib` (object library) for faster builds.

Run with environment variables:
```bash
SPDLOG_LEVEL=debug cmake --build build --target unit_tests
```

### What to test

Test business logic even when it lives in UI draw functions. Do not test ImGui rendering itself.

When a draw function contains filtering, validation, or state-transition logic, extract that logic into named free functions, declare them in the header, and test them directly.

Keep draw functions as thin callers.

### How to test 

Use Given/When/Then format for clarity. Test edge cases and failure modes, not just the happy path. Use descriptive test case names.

## Code Organization

### File Structure
- **Headers** (`*.h`): Component structs, system signatures, window classes
- **Source** (`*.cpp`): System implementations, module constructors
- **Widgets** ([src/widgets/](src/widgets/)): Reusable ImGui components

### Adding a New Module

1. Create directory: `src/modules/modulename/`
2. Add header `modulename.h`:
   ```cpp
   struct ModuleNameModule {
     ModuleNameModule(flecs::world& world);
   };
   ```
3. Implement in `modulename.cpp` (register components/systems)
4. Add files to [src/CMakeLists.txt](src/CMakeLists.txt)
5. Add `#include` and `world.import<ModuleNameModule>()` to [src/main.cpp](src/main.cpp)

### UI Windows

UI windows follow this pattern:
1. Define struct with `show()` method in module header
2. Implement ImGui code in `.cpp` (use `ImGui::Begin/End`, query ECS)
3. Call from [src/modules/engine/gui.cpp](src/modules/engine/gui.cpp) GUI system

## Configuration

- **config.lua**: User config loaded at startup (font settings, etc.)
- **imgui.ini**: ImGui window layout (auto-generated, don't edit manually)
- **.luarc.json**: Lua language server config

## Logging & Debugging

### Logging
Uses spdlog, logs to `solcorp.log`:
```cpp
spdlog::debug("message");   // Debug level
spdlog::info("message");    // Info level
spdlog::error("message");   // Error level
```

In Lua:
```lua
solcorp.logging.debug("message")
solcorp.logging.info("message")
solcorp.logging.error("message")
```

### Debugging Tools
1. **GDB**: `gdb ./build/src/solcorp` (provided in Nix shell)
2. **Flecs REST API**: Enabled via `.enable_rest()` - inspect ECS state at runtime
3. **Developer Windows**: ImGui inspector windows (see [src/modules/simulation/developer_window.h](src/modules/simulation/developer_window.h))
4. **Clangd**: LSP support for code navigation (in Nix shell)

## Architecture Decision Records (ADRs)

Major architectural decisions should be documented as ADRs in [docs/adr/](docs/adr/). Each ADR captures the context, decision, alternatives considered, and tradeoffs accepted — the "why" that isn't visible in the code.

Use the format `docs/adr/NNN-short-title.md` (e.g. `001-ecs-with-flecs.md`). A minimal ADR needs only: **Context**, **Decision**, and **Consequences**.

When proposing or reviewing a change that affects core architecture (new framework, new module pattern, data model shift, build tooling), create or reference an ADR.

## Critical Implementation Notes

### Compilation
- C++20 standard
- Strict flags: `-Wall -Wextra -Wpedantic -Werror`
- `#pragma once` for header guards

### Dependencies
Fetched via CMake FetchContent:
- Flecs v4.1.2 (ECS framework)
- spdlog (logging)
- sol3 (Lua bindings)
- Catch2 v3.6.0 (testing)

System dependencies (via Nix or package manager):
- SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
- lua5.4
- imgui

### Key Files Reference
- [src/main.cpp](src/main.cpp) - Entry point, world setup, module import order
- [src/modules/base/base.cpp](src/modules/base/base.cpp) - Phase definitions
- [src/modules/lua/lua.cpp](src/modules/lua/lua.cpp) - Sol2 state, component bindings
- [src/modules/engine/render.cpp](src/modules/engine/render.cpp) - SDL2/ImGui render pipeline
- [src/modules/engine/gui.cpp](src/modules/engine/gui.cpp) - Main GUI system
- [mods/core/init.lua](mods/core/init.lua) - Core game content and examples
