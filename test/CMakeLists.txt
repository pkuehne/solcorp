fetchcontent_declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
fetchcontent_makeavailable(catch2)

add_executable(unit_test
  main.cpp
  base/find_ancestor_with.test.cpp
  base/notification.test.cpp
  engine/apply_parent_transform.test.cpp
  engine/format_locale.test.cpp
  lua/entity/lua_push_entity.test.cpp
  lua/entity/lua_check_entity.test.cpp
  lua/entity/entity_id.test.cpp
  lua/entity/entity_is_alive.test.cpp
  lua/entity/entity_name.test.cpp
  lua/entity/entity_enabled.test.cpp
  lua/entity/entity_disable_enable.test.cpp
  lua/entity/entity_destroy.test.cpp
  lua/entity/entity_child_of.test.cpp
  lua/entity/entity_lookup.test.cpp
  lua/entity/entity_is_a.test.cpp
  lua/helpers/create_site.test.cpp
  lua/helpers/create_building_prefab.test.cpp
  lua/helpers/create_rocket_prefab.test.cpp
  lua/helpers/add_facility_to_building.test.cpp
  lua/helpers/create_rocket.test.cpp
  lua/helpers/create_building.test.cpp
  lua/helpers/add_target_orbit_to_rocket.test.cpp
  lua/helpers/create_texture.test.cpp
  lua/helpers/create_effect.test.cpp
  lua/helpers/add_modifier.test.cpp
  lua/helpers/clip_sprite_from_texture.test.cpp
  lua/helpers/create_contract.test.cpp
  lua/helpers/create_contract_payload.test.cpp
  lua/helpers/get_all_contracts.test.cpp
  lua/helpers/get_all_active_contracts.test.cpp
  lua/config/parse_config.test.cpp
  lua/logging/load_logging.test.cpp
  lua/logging/logging_levels.test.cpp
  lua/registry/world.test.cpp
  lua/registry/mod_name.test.cpp
  rocket/systems.test.cpp
  rocket/actions/cancel_launch_action.test.cpp
  rocket/actions/launch_initiate_rollout_action.test.cpp
  rocket/actions/launch_complete_rollout_action.test.cpp
  rocket/actions/launch_go_action.test.cpp
  rocket/actions/rocket_build_action.test.cpp
  rocket/actions/rocket_complete_build_action.test.cpp
  rocket/actions/rocket_complete_move_action.test.cpp
  rocket/actions/rocket_move_action.test.cpp
  rocket/actions/schedule_launch_action.test.cpp
  rocket/active_launches/plan_matches_filters.test.cpp
  rocket/contracts_window/accept_reject_contract.test.cpp
  rocket/contracts_window/setup_launch_for_payload.test.cpp
  rocket/contracts_window/displaying_contracts.test.cpp
  rocket/contracts_window/button_enabled_state.test.cpp
  simulation/update_sim_date.test.cpp
  simulation/generate_sol_system.test.cpp
  site/create_site_prefabs.test.cpp
  site/auto_start_next_build.test.cpp
  site/auto_store_built_rocket.test.cpp
  site/manufacturing_progress.test.cpp
  site/rocket_prefab_window/collect_rocket_prefabs.test.cpp
  site/rocket_prefab_window/compute_build_cost.test.cpp
  site/rocket_prefab_window/can_afford_rocket_prefab.test.cpp
  stats/stats.test.cpp
  window/building_detail_window.test.cpp
  window/notification_window.test.cpp
  window/rocket_list_window.test.cpp
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
