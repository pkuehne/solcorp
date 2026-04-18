
help:
    @just --list

init:
    cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja

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

lint: lint-cpp lint-lua 

lint-cpp:

lint-lua:
    luacheck config.lua mods/

docs:
    python3 -m http.server 8000 --directory docs
