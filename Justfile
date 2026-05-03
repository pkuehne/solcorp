set windows-shell := ["powershell.exe", "-NoProfile", "-Command"]

build_dir := "build"

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
    cmake --preset release-no-tests
    cmake --build {{build_dir}} --parallel
    cmake --install {{build_dir}} --prefix dist

package:
    cpack --config {{build_dir}}/CPackConfig.cmake -B {{build_dir}}

build:
    cmake --build {{build_dir}} --target all --parallel

test:
    cmake --build {{build_dir}} --target unit_tests --parallel

run:
    cmake --build {{build_dir}} --target run --parallel

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
    find src test -name "*.cpp" -print0 | xargs -0 -r -n 1 -P "$(nproc)" clang-tidy --quiet -p {{build_dir}}

lint-lua:
    luacheck config.lua mods/

docs:
    python3 -m http.server 8000 --directory docs
