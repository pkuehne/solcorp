# Architecture

SolCorp is a C++20 game built on an Entity Component System (ECS) using [Flecs v4.1.2](https://www.flecs.dev/).

## Contents

- [ECS & Modules](architecture/ecs-and-modules.md) — How the module system works and how to add a module
- [Systems & Phases](architecture/systems-and-phases.md) — Execution order and the 11-phase game loop
- [Flecs Relationships](architecture/flecs-relationships.md) — How parent-child and custom relationships are used
- [UI & Widgets](architecture/ui-and-widgets.md) — ImGui integration and the window pattern

See also the [ADRs](../adr/README.md) for recorded design decisions.

## Tech Stack

| Concern | Library |
|---------|---------|
| ECS | Flecs v4.1.2 |
| Rendering | SDL2 + SDL2_image |
| UI | ImGui |
| Lua scripting | sol2 (sol3) + lua 5.4 |
| Logging | spdlog |
| Testing | Catch2 v3.6.0 |
| Build | CMake + Ninja + Justfile |
| Dev environment | Nix flake |
