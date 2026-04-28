cmake_minimum_required(VERSION 3.21)

# Normalize mixed-separator DLL paths before regex matching during install
if(POLICY CMP0207)
  cmake_policy(SET CMP0207 NEW)
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

set(SC_VERSION 0.1.0) # x-release-please-version
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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
  add_compile_options(/diagnostics:color)
else()
  add_compile_options(-fdiagnostics-color=always)
endif()

include(FetchContent)

add_subdirectory(external)
add_subdirectory(src)

if(SC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

set(CPACK_PACKAGE_NAME "SolCorp")
set(CPACK_PACKAGE_VERSION ${SC_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_GENERATOR "ZIP")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
