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
  contracts_window.test.cpp
  base_tests.cpp
  lua_entity.test.cpp
  lua_helpers.test.cpp
  lua_logging.test.cpp
  lua_registry.test.cpp
  rocket/systems.test.cpp
  rocket/actions/cancel_launch_action.test.cpp
  rocket/actions/edit_launch_action.test.cpp
  rocket/actions/rocket_build_action.test.cpp
  rocket/actions/rocket_complete_build_action.test.cpp
  rocket/actions/rocket_complete_move_action.test.cpp
  rocket/actions/rocket_move_action.test.cpp
  rocket/actions/schedule_launch_action.test.cpp
  simulation.test.cpp
  site.test.cpp
  stats.test.cpp
  rocket_prefab_window.test.cpp
)
target_link_libraries(unit_test PRIVATE Catch2::Catch2)
target_link_libraries(unit_test PRIVATE solcorplib)
if(MSVC)
  target_compile_options(unit_test PUBLIC /W4)
else()
  target_compile_options(unit_test PUBLIC -Wall)
endif()
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
