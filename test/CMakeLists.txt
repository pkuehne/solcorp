fetchcontent_declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
fetchcontent_makeavailable(catch2)

add_executable(unit_test
  main.cpp
  manufacturing.test.cpp
  render.test.cpp
  active_launches_tests.cpp
  base_tests.cpp
  rocket_launch_tests.cpp
  simulation.test.cpp
  site.test.cpp
  stats.test.cpp
)
target_link_libraries(unit_test PRIVATE Catch2::Catch2)
target_link_libraries(unit_test PRIVATE solcorplib)
target_compile_options(unit_test PUBLIC -Wall)
target_compile_definitions(unit_test PRIVATE
                            SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

# Register tests with CTest
include(Catch)
catch_discover_tests(
  unit_test
  EXTRA_ARGS --colour-mode ansi
  PROPERTIES
    ENVIRONMENT "SPDLOG_LEVEL=debug"
)
# set_tests_properties(unit_test PROPERTIES ENVIRONMENT "SPDLOG_LEVEL=debug")

add_custom_target(tests
  DEPENDS unit_test
  COMMAND ${CMAKE_COMMAND} -E env
          SPDLOG_LEVEL=debug
          CTEST_OUTPUT_ON_FAILURE=1
          ${CMAKE_CTEST_COMMAND} -C $<CONFIG> # -V
  USES_TERMINAL
  COMMENT "Running CTest"
)

add_custom_target(unit_tests
  DEPENDS unit_test
  COMMAND ${CMAKE_COMMAND} -E env
          SPDLOG_LEVEL=debug
          "$<TARGET_FILE:unit_test>"
          --colour-mode ansi
          ${TEST_ARGS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  USES_TERMINAL
  COMMENT "Running unit_test executable"
)
