# Systems & Phases

## Execution Model

The game runs at 60 FPS via `world.app().enable_stats().enable_rest().run()`. Each frame Flecs executes all registered systems in phase order.

## Phases

Phases are defined in `BaseModule` and executed in this order each frame:

| # | Phase | Purpose |
|---|-------|---------|
| 1 | `PostStartPhase` | One-time initialization after world start |
| 2 | `PreFramePhase` | Per-frame setup (clear state, poll input) |
| 3 | `UpdatePhase` | Game logic (AI, state machines, contracts) |
| 4 | `PhysicsPhase` | Movement and orbital simulation |
| 5 | `GuiPhase` | ImGui UI construction |
| 6 | `RenderPhase` | SDL2 draw calls |
| 7 | `PostFramePhase` | Frame cleanup |

*(Phases 8–11 are defined in BaseModule — document as they are used)*

## Registering a System

```cpp
world.system<ComponentA, ComponentB>("MySystem")
    .kind(UpdatePhase)
    .iter([](flecs::iter& it, ComponentA* a, ComponentB* b) {
        for (auto i : it) {
            // process entity i
        }
    });
```

## Lua Systems

Lua mods hook into the game loop via handlers rather than registering Flecs systems directly:

```lua
solcorp.script.handlers.on_update = function() end  -- runs in UpdatePhase
solcorp.script.handlers.on_frame  = function() end  -- runs in GuiPhase
```
