fetchcontent_declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        main
)
fetchcontent_makeavailable(googletest)

add_executable(unit_test
    render.test.cpp
    systems.test.cpp
    simulation.test.cpp
    stats.test.cpp
)

target_compile_options(unit_test PUBLIC -Wall)
target_link_libraries(unit_test gtest_main gmock)
target_link_libraries(unit_test solcorplib)

add_test(
    NAME unit_test
    COMMAND unit_test --gtest_shuffle
)

add_custom_target(tests
    DEPENDS unit_test
    COMMAND unit_test --gtest_shuffle
    COMMENT "Running tests"
)
