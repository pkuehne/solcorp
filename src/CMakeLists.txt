list(APPEND SolCorpSources
    widgets/widgets.cpp
    modules/base/base.cpp
    modules/engine/engine.cpp
    modules/engine/gui.cpp
    modules/engine/helpers.cpp
    modules/engine/input.cpp
    modules/engine/movement.cpp
    modules/engine/render.cpp
    modules/lua/entity.cpp
    modules/lua/helpers.cpp
    modules/lua/lua.cpp
    modules/lua/logging.cpp
    modules/lua/systems.cpp
    modules/main_menu/main_menu.cpp
    modules/rocket_launch/rocket_launch.cpp
    modules/rocket_launch/actions.cpp
    modules/rocket_launch/launch_window.cpp
    modules/rocket_launch/active_launches_window.cpp
    modules/simulation/celestial_browser.cpp
    modules/simulation/developer_window.cpp
    modules/simulation/simulation.cpp
    modules/site/building_window.cpp
    modules/site/construction_window.cpp
    modules/site/helpers.cpp
    modules/site/site.cpp
    modules/site/site_construction.cpp
    modules/staff/staff.cpp
    modules/stats/stats.cpp
)
list(APPEND SolCorpHeaders
    widgets/widgets.h
    modules/base/base.h
    modules/base/assert.h
    modules/engine/engine.h
    modules/engine/gui.h
    modules/engine/helpers.cpp
    modules/engine/input.h
    modules/engine/movement.h
    modules/engine/render.h
    modules/lua/entity.h
    modules/lua/helpers.h
    modules/lua/lua.h
    modules/lua/logging.h
    modules/lua/systems.h
    modules/main_menu/main_menu.h
    modules/rocket_launch/rocket_launch.h
    modules/rocket_launch/actions.h
    modules/rocket_launch/launch_window.h
    modules/rocket_launch/active_launches_window.h
    modules/simulation/celestial_browser.h
    modules/simulation/developer_window.h
    modules/simulation/simulation.h
    modules/site/building_window.h
    modules/site/construction_window.h
    modules/site/helpers.h
    modules/site/site.h
    modules/site/site_construction.h
    modules/staff/staff.h
    modules/stats/stats.h
)

fetchcontent_declare(flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs
    GIT_TAG v4.1.2
)
fetchcontent_declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog/
    GIT_TAG v1.x
)
fetchcontent_declare(sol3
  GIT_REPOSITORY https://github.com/ThePhD/sol2/
    GIT_TAG develop
)


set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

fetchcontent_makeavailable(flecs spdlog sol3)

add_compile_definitions(SC_VERSION="${PROJECT_VERSION}")

# Add SDL2 depndencies
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
pkg_search_module(SDL2TTF REQUIRED SDL2_ttf>=2.0.14)
pkg_search_module(LUA54 REQUIRED lua5.4)

# Library
# Create an object library that is not linked, but serves as a collection of
# objects for other libraries
add_library(solcorplib OBJECT "${SolCorpSources}")
add_library(solcorp::solcorplib ALIAS solcorplib)

target_include_directories(solcorplib PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(solcorplib PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${SDL2IMAGE_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${SDL2TTF_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${LUA54_INCLUDE_DIRS})

set_target_properties(solcorplib PROPERTIES PUBLIC_HEADER "${SolCorpHeaders}")
target_compile_definitions(solcorplib PRIVATE
                            SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)


target_compile_options(solcorplib PRIVATE -Werror)
target_compile_options(solcorplib PRIVATE -Wall)
target_compile_options(solcorplib PRIVATE -Wextra)
target_compile_options(solcorplib PRIVATE -Wpedantic)
target_compile_options(solcorplib PRIVATE ${LUA54_CFLAGS})

target_link_libraries(solcorplib ${SDL2_LINK_LIBRARIES})
target_link_libraries(solcorplib ${SDL2IMAGE_LINK_LIBRARIES})
target_link_libraries(solcorplib ${SDL2TTF_LINK_LIBRARIES})
target_link_libraries(solcorplib flecs::flecs_static)
target_link_libraries(solcorplib spdlog::spdlog)
target_link_libraries(solcorplib imgui)
target_link_libraries(solcorplib sol2::sol2)
target_link_libraries(solcorplib ${LUA54_LINK_LIBRARIES})

# Embedded textures
find_program(XXD xxd REQUIRED)
set(RES_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/../textures/construction.png)
set(RES_GEN  ${CMAKE_CURRENT_BINARY_DIR}/construction_png.cpp)
add_custom_command(
  OUTPUT  ${RES_GEN}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND ${XXD} -i -n construction_png ${RES_SRC} > ${RES_GEN}
  DEPENDS ${RES_SRC}
  VERBATIM
  COMMENT "Embedding textures/construction.png -> construction_png.cpp"
)

# Object library to keep things tidy
add_library(resources STATIC ${RES_GEN})
target_include_directories(resources PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(solcorplib resources)

# Executable
add_executable(solcorp main.cpp)

target_include_directories(solcorp PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_options(solcorp PRIVATE -Werror)
target_compile_options(solcorp PRIVATE -Wall)
target_compile_options(solcorp PRIVATE -Wextra)
target_compile_options(solcorp PRIVATE -Wpedantic)

target_link_libraries(solcorp PRIVATE solcorplib)

file(GLOB TEXTURES "${CMAKE_SOURCE_DIR}/textures/*.png")
file(COPY ${TEXTURES}
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/textures)

add_custom_target(run
    COMMAND solcorp
    DEPENDS solcorp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT Running SolCorp
)

add_custom_target(debug
    COMMAND gdb $<TARGET_FILE:solcorp>
    USES_TERMINAL
    DEPENDS solcorp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT Debugging SolCorp
)
