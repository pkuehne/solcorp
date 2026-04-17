build:
    cd build && ninja

test:
    cd build && ninja unit_tests

run:
    cd build && ninja run