# ECS & Modules

## Entity Component System

SolCorp uses Flecs as its ECS. The core primitives are:

- **Entity** — an ID; the "thing" (a rocket, a building, a celestial body)
- **Component** — plain data struct attached to an entity (e.g. `Transform`, `Sprite`, `Launchpad`)
- **System** — a function that queries for entities with specific components and runs each game loop phase

## Module System

The game is structured as 9 independent modules. Each module is a C++ struct whose constructor registers that module's components, systems, observers, and prefabs with the Flecs world.

| Module | Responsibility |
|--------|---------------|
| `BaseModule` | Shared phases and global entities — **must be imported first** |
| `LuaModule` | sol2 state, component bindings, mod loading |
| `EngineModule` | SDL2 rendering, input, camera, movement |
| `StatsModule` | Statistics tracking |
| `SimulationModule` | Celestial mechanics, orbital calculations |
| `MainMenuModule` | Main menu UI |
| `SiteModule` | Construction sites, buildings, facilities |
| `RocketLaunchModule` | Launch contracts and operations |
| `StaffModule` | Crew and personnel |

### Import Order

Modules are imported in `src/main.cpp` in dependency order. `BaseModule` must come first (it defines shared phases). `LuaModule` comes early so that other modules can bind their components to Lua. The rest can be in any order, but it's good practice to keep related modules together (e.g. `SiteModule` and `RocketLaunchModule`).

Modules import each other as needed. For example, `SiteModule` imports `StatsModule` because buildings have stats. `MainMenuModule` imports `LuaModule` to expose menu controls to Lua scripts. Modules should avoid circular dependencies. If two modules need to reference each other's components, consider defining those components in `BaseModule` or a new shared module.

## Adding a Module

1. Create `src/modules/<name>/`
2. Write the header with a constructor taking `flecs::world&`
3. Implement in `<name>.cpp` — register components and systems
4. Add both files to `src/CMakeLists.txt`
5. `#include` the header and call `world.import<YourModule>()` in `src/main.cpp`

## Component Definition

1. Define a plain struct in the module header
2. Register it in the module constructor: `world.component<YourComponent>()` alongside its member variables. This allows Flecs to reflect on the component for queries and serialization as well the developer tools.
3. To expose to Lua: call `register_lua_user_type<YourComponent>()` in `src/modules/lua/lua.cpp`
