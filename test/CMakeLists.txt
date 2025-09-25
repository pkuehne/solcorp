fetchcontent_declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
fetchcontent_makeavailable(catch2)

add_executable(unit_test
  render.test.cpp
  rocket_launch_tests.cpp
  simulation.test.cpp
  stats.test.cpp
)
target_link_libraries(unit_test PRIVATE Catch2::Catch2WithMain)
target_link_libraries(unit_test PRIVATE solcorplib)
target_compile_options(unit_test PUBLIC -Wall)

# Register tests with CTest
include(Catch)
catch_discover_tests(
  unit_test
  EXTRA_ARGS --colour-mode ansi
)

add_custom_target(tests
    DEPENDS unit_test
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running tests"
)

