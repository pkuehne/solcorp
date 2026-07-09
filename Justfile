set windows-shell := ["powershell.exe", "-NoProfile", "-Command"]

build_dir := "build"
aseprite := "/mnt/c/Program Files (x86)/Steam/steamapps/common/Aseprite/aseprite.exe"

# List available commands
help:
    @just --list

# Configure for debug (default)
configure:
    cmake --preset {{ if os() == "windows" { "windows-debug" } else { "linux-debug" } }}

# Configure for debug
configure-debug: configure

# Configure for release
configure-release:
    cmake --preset {{ if os() == "windows" { "windows-release" } else { "linux-release" } }}

# Configure for release and don't build tests
configure-release-no-tests:
    cmake --preset {{ if os() == "windows" { "windows-release-no-tests" } else { "linux-release-no-tests" } }}

# Configure for debug with sanitizers
configure-sanitize:
    cmake --preset linux-sanitize

# Configure, build, and install a release build to dist/
install:
    just configure-release-no-tests
    cmake --build {{ build_dir }} --parallel
    cmake --install {{ build_dir }} --prefix dist

# Package the build with CPack
package:
    cpack --config {{ build_dir }}/CPackConfig.cmake -B {{ build_dir }}

# Build the entire project
build:
    cmake --build {{ build_dir }} --target all --parallel

# Build and run tests
test:
    cmake --build {{ build_dir }} --target unit_tests --parallel

# Build and run the game
run:
    cmake --build {{ build_dir }} --target run --parallel

# Clean build files
clean:
    cmake --build {{ build_dir }} --target clean

# Format all C++ and Lua source files
format: format-cpp format-lua

# Format C++ source files
format-cpp:
    find src test \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 -r clang-format -i

# Format Lua source files
format-lua:
    stylua config.lua mods/

# Check formatting for all C++ and Lua source files
check-format: check-format-cpp check-format-lua

# Check C++ formatting without modifying files
check-format-cpp:
    find src test \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 -r clang-format --dry-run -Werror

# Check Lua formatting without modifying files
check-format-lua:
    stylua --check config.lua mods/

# Lint changed C++ (uncommitted), all Lua, and all shell sources (fast local default)
lint: lint-cpp-changed lint-lua lint-shell

# Lint every C++, Lua, and shell source file (full sweep; run by the merge/main CI)
lint-all: lint-cpp lint-lua lint-shell

# Lint all C++ source files with clang-tidy (full sweep)
lint-cpp:
    BUILD_DIR={{ build_dir }} scripts/lint-cpp.sh --all

# Lint C++ translation units with uncommitted changes (staged, unstaged, or untracked)
lint-cpp-changed:
    BUILD_DIR={{ build_dir }} scripts/lint-cpp.sh

# Lint C++ translation units this branch changed vs a base git ref (used by the PR CI)
lint-cpp-since base:
    BUILD_DIR={{ build_dir }} scripts/lint-cpp.sh --since {{ base }}

# Lint Lua source files with luacheck
lint-lua:
    luacheck config.lua mods/

# Lint shell scripts with shellcheck
lint-shell:
    find scripts -name "*.sh" -print0 | xargs -0 -r shellcheck

# Lint C++ source files with clang-tidy and fix any auto-fixable issues
lint-fix-cpp:
    find src test -name "*.cpp" -print0 | xargs -0 -r -n 1 clang-tidy --quiet -p {{ build_dir }} --fix

# Serve documentation locally
docs:
    python3 -m http.server 8000 --directory docs

# Export tilesets from Aseprite files in the assets/aseprite directory to PNG files in the assets/textures directory
export-tilesets:
    "{{ aseprite }}" -b --layer "road_asphalt" assets/aseprite/solcorp_roads.aseprite --export-tileset --sheet assets/textures/roads_asphalt.png
    "{{ aseprite }}" -b --layer "road_markings" assets/aseprite/solcorp_roads.aseprite --export-tileset --sheet assets/textures/roads_markings.png
