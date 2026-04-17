
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