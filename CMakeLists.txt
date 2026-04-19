cmake_minimum_required(VERSION 3.13)

set(SC_VERSION 0.0.1) # x-release-please-version
project(SolCorp
    VERSION ${SC_VERSION}
    DESCRIPTION "Sol Corp"
    HOMEPAGE_URL "https://github.com/pkuehne/solcorp/"
    LANGUAGES CXX
)

message("Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")
option(SC_BUILD_TESTS "Whether to build the unit tests" ON)
option(SC_LINT_COMPILATION "Whether to lint as we build with clang-tidy" OFF)
set(COVERAGE_PERCENTAGE 90 CACHE STRING "Minimum Coverage number to succeed")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-fdiagnostics-color=always)

include(FetchContent)

add_subdirectory(external)
add_subdirectory(src)

if(SC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
