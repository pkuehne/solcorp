list(APPEND SolCorpSources
    widgets/widgets.cpp
    modules/gui.cpp
    modules/input.cpp
    modules/main_menu.cpp
    modules/phase.cpp
    modules/rocket_launch.cpp
    modules/render.cpp
    modules/simulation.cpp
    modules/site.cpp
    modules/staff.cpp
    )
list(APPEND SolCorpHeaders
    widgets/widgets.h
    modules/gui.h
    modules/input.h
    modules/main_menu.h
    modules/phase.h
    modules/rocket_launch.h
    modules/render.h
    modules/simulation.h
    modules/site.h
    modules/staff.h
    )

fetchcontent_declare(flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs
    GIT_TAG v4.0.0
)
fetchcontent_declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog/
    GIT_TAG v1.x
)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

fetchcontent_makeavailable(flecs spdlog)

add_compile_definitions(SC_VERSION="${PROJECT_VERSION}")

# Add SDL2 depndencies
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
pkg_search_module(SDL2TTF REQUIRED SDL2_ttf>=2.0.14)

# Library
# Create an object library that is not linked, but serves as a collection of
# objects for other libraries
add_library(solcorplib OBJECT "${SolCorpSources}")
add_library(solcorp::solcorplib ALIAS solcorplib)

target_include_directories(solcorplib PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(solcorplib PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${SDL2IMAGE_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${SDL2TTF_INCLUDE_DIRS})

set_target_properties(solcorplib PROPERTIES PUBLIC_HEADER
    "${SolCorpHeaders}")

target_compile_options(solcorplib PRIVATE -Werror)
target_compile_options(solcorplib PRIVATE -Wall)
target_compile_options(solcorplib PRIVATE -Wextra)
target_compile_options(solcorplib PRIVATE -Wpedantic)

target_link_libraries(solcorplib ${SDL2_LIBRARIES})
target_link_libraries(solcorplib ${SDL2IMAGE_LIBRARIES})
target_link_libraries(solcorplib ${SDL2TTF_LIBRARIES})
target_link_libraries(solcorplib flecs::flecs_static)
target_link_libraries(solcorplib spdlog::spdlog)
target_link_libraries(solcorplib imgui)

# Executable
add_executable(solcorp main.cpp)

target_include_directories(solcorp PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_options(solcorp PRIVATE -Werror)
target_compile_options(solcorp PRIVATE -Wall)
target_compile_options(solcorp PRIVATE -Wextra)
target_compile_options(solcorp PRIVATE -Wpedantic)

target_link_libraries(solcorp PRIVATE solcorplib)

add_custom_target(run
    COMMAND solcorp
    DEPENDS solcorp
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
