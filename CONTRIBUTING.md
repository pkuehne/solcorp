# Contributing to Sol, Corp.

Thanks for your interest in contributing! This guide covers everything you need to get started.

## Getting Started

### Linux

The project uses a [Nix flake](https://nixos.org/) for a fully reproducible development environment. If you have Nix installed:

```bash
nix develop          # enter the dev shell (provides all tools and libraries)
just init            # configure CMake
just build           # compile
just test            # run tests
just run             # run the game
```

If you don't have Nix, you'll need to install the system dependencies manually: SDL2, SDL2\_image, SDL2\_ttf, SDL2\_mixer, lua5.4, and imgui. Then use the same `just` commands above.

### Windows

Windows builds use [vcpkg](https://vcpkg.io/) for dependencies:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --parallel
```

## Development Workflow

1. Fork the repository and create a branch from `main`.
2. Make your changes, add tests where appropriate (see [Testing](#testing) below).
3. Run `just format` and `just lint` before pushing — CI will check both.
4. Open a pull request against `main`.

### PR Titles

PR titles must follow [Conventional Commits](https://www.conventionalcommits.org/) — CI enforces this. Allowed types:

| Type | Use for |
|---|---|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `perf` | Performance improvement |
| `refactor` | Code restructuring with no behavior change |
| `test` | Adding or fixing tests |
| `docs` | Documentation only |
| `chore` | Build system, dependencies, tooling |
| `revert` | Reverting a previous commit |

Example: `feat(simulation): add Hohmann transfer orbit calculation`

## Code Style

C++ is formatted with `clang-format` and Lua with `stylua`. Run both with:

```bash
just format
```

Lua is also linted with `luacheck`:

```bash
just lint
```

These are all enforced in CI, so it's worth running them locally before pushing.

## Testing

Tests use [Catch2](https://github.com/catchorg/Catch2) and live in [test/](test/).

```bash
just test
# or, with debug logging:
SPDLOG_LEVEL=debug just test
```

A few conventions to follow:

- Use **Given/When/Then** structure for test cases — it makes intent clear.
- Test edge cases and failure modes, not just the happy path.
- Use descriptive test case names.
- **Don't test ImGui rendering directly.** If a draw function contains filtering, validation, or state-transition logic, extract that logic into a named free function, declare it in the header, and test it there. Keep draw functions as thin callers.

## Architecture

Sol, Corp. is built on an **Entity Component System** using [Flecs](https://github.com/SanderMertens/flecs). The codebase is structured as independent modules under [src/modules/](src/modules/), each registering its own components and systems.

If you're making a significant architectural change — adding a new framework, changing a core module pattern, restructuring the data model — please document the decision as an **Architecture Decision Record** in [docs/adr/](docs/adr/). A minimal ADR just needs context, the decision, and the consequences. See existing ADRs for examples.

The [CLAUDE.md](CLAUDE.md) file has a detailed breakdown of module structure, component patterns, system registration, and Lua integration if you want to go deeper.

## Reporting Issues

Please use [GitHub Issues](https://github.com/pkuehne/solcorp/issues) to report bugs or request features. When filing a bug, include your OS, steps to reproduce, and the relevant section of `solcorp.log` if applicable.
