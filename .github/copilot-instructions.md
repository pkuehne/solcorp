# SolCorp AI Coding Agent Instructions

## Project Overview
SolCorp is a C++20/CMake-based space simulation game using an **Entity Component System (ECS)** architecture with Flecs. The project features modular game logic, ImGui UI, Lua scripting, and SDL2 graphics. Build system uses CMake with Ninja, and tests use Catch2.

## Architecture

### ECS-Based Module System
- **Core Framework**: Flecs v4.1.2 (fast, lightweight ECS)
- **Structure**: Nine modules (`src/modules/*/`) that define game domains (engine, simulation, site, staff, etc.)
- **Module Pattern**: Each module is a struct with a constructor taking `flecs::world&` that registers components, systems, and phases
  - Example: [src/modules/base/base.h](src/modules/base/base.h) defines shared phases (PostStartPhase, PreFramePhase, etc.)
  - Each module imports into world via `world.import<ModuleName>()`
- **Execution Model**: Systems run in 11 defined phases (PostStart → PostFrame). GUI runs in GuiPhase, rendering in RenderPhase
- **Component Registration**: Define struct in `.h` file (e.g., `struct CelestialBody { ... }`); add to component list in module

### Key Integration Points
1. **Main Loop**: [src/main.cpp](src/main.cpp) initializes world, imports all modules, runs at 60 FPS via `world.app().enable_stats().enable_rest().run()`
2. **Lua Integration** ([src/modules/lua/lua.h](src/modules/lua/lua.h)): Uses sol2 (Lua C++ bindings)
   - Lua components exposed via `register_lua_user_type<T>()` creates getters/setters/hasers in `solcorp.components` namespace
   - Config loaded via `load_config_file()` (reads [config.lua](config.lua))
3. **UI**: ImGui for windows; implemented in [src/modules/engine/gui.cpp](src/modules/engine/gui.cpp) and per-module windows
4. **Logging**: spdlog; root logger set to debug level in [main.cpp](src/main.cpp), logs to `solcorp.log`

### Module Responsibilities
- **base**: Defines shared phases and global entities (PostStartPhase, UpdatePhase, etc.)
- **engine**: Rendering, input, movement, camera (SDL2 + ImGui)
- **lua**: Component exposure, script loading, Lua mod system
- **simulation**: Celestial mechanics, orbital calculations (CelestialBody components)
- **site/staff/stats/rocket_launch/main_menu**: Domain-specific game logic

## Build & Development

### Build Commands (via CMake/Ninja)
```bash
cmake -B build -G Ninja        # Configure
cmake --build build            # Compile (debug)
cmake --build build -c Release # Release build
cmake --build build --target unit_tests  # Run unit tests
```

### Development Environment
- **Nix Flake**: [flake.nix](flake.nix) provides reproducible dev shell with SDL2, lua5.4, imgui, catch2, gcc, gdb, clangd
- **Fetched Dependencies**: Flecs, spdlog, sol3 (Lua), Catch2 – all via FetchContent in CMake
- **Compile Settings**: C++20 standard, errors on warnings (`-Werror`), strict flags (`-Wall -Wextra -Wpedantic`)

### Testing
- **Framework**: Catch2 v3.6.0
- **Test Files**: [test/](test/) – one `.test.cpp` per domain (e.g., `simulation.test.cpp`, `site.test.cpp`)
- **Running**: `cmake --build build --target unit_tests` (env var `SPDLOG_LEVEL=debug`, `TEST_ARGS` for filters)
- **Linking**: Tests link `solcorplib` (object library containing compiled source) for faster builds than static lib

### Code Organization
- **Headers** (`*.h`): Pragma once guards, component structs + system signatures
- **Source** (`*.cpp`): System implementations, module constructors
- **Build Artifacts**: Embedded texture `construction.png` → `construction_png.cpp` via xxd
- **ImGui Config**: [imgui.ini](imgui.ini) auto-generated at runtime

## Common Patterns & Conventions

### Flecs System Registration
```cpp
// In module constructor
world.system<ComponentType>("system_name")
  .kind(UpdatePhase)  // or GuiPhase, RenderPhase, etc.
  .iter(systemImplementationFunction);
```

### Component Definition
1. Define struct in module header (e.g., [src/modules/simulation/simulation.h](src/modules/simulation/simulation.h) defines `CelestialBody`, `Simulation`, `Game`)
2. Add to [src/CMakeLists.txt](src/CMakeLists.txt) source lists if implementing systems for it
3. If Lua-exposed: Call `register_lua_user_type<ComponentType>()` in lua module

### Lua Component Access Pattern
```lua
local pos = entity:getPosition()
entity:setPosition(pos)
if entity:hasPosition() then ... end
entity:removePosition()
```

### Error Handling
- Assertions via [src/modules/base/assert.h](src/modules/base/assert.h)
- Spdlog logger: `spdlog::debug("msg")`, `spdlog::error("msg")`
- Never silently fail; log errors to aid debugging

### UI Windows
- Define struct (e.g., `CelestialBrowserWindow`) with `show()` method in `.h`
- Implement ImGui code in `.cpp` (query components, render imgui::Begin/End)
- Call from [src/modules/engine/gui.cpp](src/modules/engine/gui.cpp) main GUI system

### Adding New Module
1. Create `src/modules/modulename/` directory
2. Add `modulename.h` with struct `ModuleNameModule { ModuleNameModule(flecs::world&); }`
3. Implement in `modulename.cpp` (register components, systems)
4. Add files to [src/CMakeLists.txt](src/CMakeLists.txt) source/header lists
5. Add `#include` and `world.import<ModuleNameModule>()` in [src/main.cpp](src/main.cpp)

## Critical Files & References
- **Entry Point**: [src/main.cpp](src/main.cpp) – world setup, module import order
- **Base Phases**: [src/modules/base/base.cpp](src/modules/base/base.cpp) – phase registration
- **ECS Concepts**: Flecs documentation (v4.1.2); systems run per-phase in world lifecycle
- **Lua Binding**: [src/modules/lua/lua.cpp](src/modules/lua/lua.cpp) – sol2 state setup, component binding
- **Render Pipeline**: [src/modules/engine/render.cpp](src/modules/engine/render.cpp) – SDL2 rendering, ImGui integration
- **Test Build Setup**: [test/CMakeLists.txt](test/CMakeLists.txt) – Catch2 discovery, environment vars

## Debugging Tips
1. **Log Level**: Set `SPDLOG_LEVEL=debug` env var to see debug logs (main.cpp sets debug level by default)
2. **GDB**: Use `gdb ./build/src/solcorp` (available in flake.nix)
3. **Clangd**: Enable code analysis in editor (in flake.nix tools)
4. **REST API**: Flecs exposes REST API (`.enable_rest()`) – useful for inspecting ECS state during runtime
5. **ImGui Inspector**: Developer windows ([src/modules/simulation/developer_window.h](src/modules/simulation/developer_window.h)) useful for debugging game state
