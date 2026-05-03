set windows-shell := ["powershell.exe", "-NoProfile", "-Command"]

help:
    @just --list

configure:
    cmake --preset debug

configure-debug: configure

configure-release:
    cmake --preset release

configure-sanitize:
    cmake --preset sanitize

install:
    cmake --preset release-no-tests -B build-release
    cmake --build build-release --parallel
    cmake --install build-release --prefix dist

build:
    cmake --build build --target all --parallel

test:
    cmake --build build --target unit_tests --parallel

run:
    cmake --build build --target run --parallel

format: format-cpp format-lua

format-cpp:
    find src test \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 -r clang-format -i

format-lua:
    stylua config.lua mods/

check-format: check-format-cpp check-format-lua

check-format-cpp:
    find src test \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 -r clang-format --dry-run -Werror

check-format-lua:
    stylua --check config.lua mods/

lint: lint-cpp lint-lua 

lint-cpp:
    find src test -name "*.cpp" -print0 | xargs -0 -r -n 1 -P "$(nproc)" clang-tidy --quiet -p build

lint-lua:
    luacheck config.lua mods/

docs:
    python3 -m http.server 8000 --directory docs
