set windows-shell := ["powershell.exe", "-NoProfile", "-Command"]

build_dir := "build"

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

# Lint all C++ and Lua source files
lint: lint-cpp lint-lua

# Lint C++ source files with clang-tidy
lint-cpp:
    find src test -name "*.cpp" -print0 | xargs -0 -r -n 1 -P "$(nproc)" clang-tidy --quiet -p {{ build_dir }}

# Lint Lua source files with luacheck
lint-lua:
    luacheck config.lua mods/

# Serve documentation locally
docs:
    python3 -m http.server 8000 --directory docs
