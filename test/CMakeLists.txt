fetchcontent_declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        main
)

fetchcontent_getproperties(googletest)
if(NOT googletest_POPULATED)
  message(STATUS "Downloading googletest repo")
  fetchcontent_populate(googletest)
  message(STATUS "Downloading googletest repo - done")
  add_subdirectory(
        ${googletest_SOURCE_DIR} ${googletest_BINARY_DIR}
        EXCLUDE_FROM_ALL
    )
endif()

add_executable(unit_test
    systems.test.cpp
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
    )
