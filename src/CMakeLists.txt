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
    modules/lua/lua_registry.cpp
    modules/lua/systems.cpp
    modules/main_menu/main_menu.cpp
    modules/rocket/rocket_launch.cpp
    modules/rocket/actions.cpp
    modules/rocket/launch_window.cpp
    modules/rocket/active_launches_window.cpp
    modules/rocket/contracts_window.cpp
    modules/simulation/celestial_browser.cpp
    modules/simulation/developer_window.cpp
    modules/simulation/simulation.cpp
    modules/site/building_window.cpp
    modules/site/construction_window.cpp
    modules/site/rocket_prefab_window.cpp
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
    modules/lua/lua_registry.h
    modules/lua/systems.h
    modules/main_menu/main_menu.h
    modules/rocket/rocket_launch.h
    modules/rocket/actions.h
    modules/rocket/launch_window.h
    modules/rocket/active_launches_window.h
    modules/rocket/contracts_window.h
    modules/simulation/celestial_browser.h
    modules/simulation/developer_window.h
    modules/simulation/simulation.h
    modules/site/building_window.h
    modules/site/construction_window.h
    modules/site/rocket_prefab_window.h
    modules/site/helpers.h
    modules/site/site.h
    modules/site/site_construction.h
    modules/staff/staff.h
    modules/stats/stats.h
)

fetchcontent_declare(flecs
    SYSTEM
    GIT_REPOSITORY https://github.com/SanderMertens/flecs
    GIT_TAG v4.1.2
)
fetchcontent_declare(spdlog
    SYSTEM
    GIT_REPOSITORY https://github.com/gabime/spdlog/
    GIT_TAG v1.x
)

set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

fetchcontent_makeavailable(flecs spdlog)

add_compile_definitions(SC_VERSION="${PROJECT_VERSION}")

# Add SDL2 dependencies
if(WIN32)
  find_package(SDL2 REQUIRED CONFIG)
  find_package(SDL2_image REQUIRED CONFIG)
  find_package(SDL2_ttf REQUIRED CONFIG)
  find_package(Lua 5.4 REQUIRED)
  set(SDL2_INCLUDE_DIRS "")
  set(SDL2_LINK_LIBRARIES SDL2::SDL2)
  set(SDL2IMAGE_INCLUDE_DIRS "")
  set(SDL2IMAGE_LINK_LIBRARIES SDL2_image::SDL2_image)
  set(SDL2TTF_INCLUDE_DIRS "")
  set(SDL2TTF_LINK_LIBRARIES SDL2_ttf::SDL2_ttf)
  set(LUA54_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
  set(LUA54_LINK_LIBRARIES ${LUA_LIBRARIES})
  set(LUA54_CFLAGS "")
else()
  include(FindPkgConfig)
  pkg_search_module(SDL2 REQUIRED sdl2)
  pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
  pkg_search_module(SDL2TTF REQUIRED SDL2_ttf>=2.0.14)
  pkg_search_module(LUA54 REQUIRED lua5.4)
endif()

# OBJECT type shares compiled objects with test targets (faster incremental debug builds).
# STATIC type absorbs into the executable at link time (single binary, no separate lib).
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_library(solcorplib STATIC "${SolCorpSources}")
else()
  add_library(solcorplib OBJECT "${SolCorpSources}")
endif()
add_library(solcorp::solcorplib ALIAS solcorplib)

target_include_directories(solcorplib PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(solcorplib PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${SDL2IMAGE_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${SDL2TTF_INCLUDE_DIRS})
target_include_directories(solcorplib PUBLIC ${LUA54_INCLUDE_DIRS})

set_target_properties(solcorplib PROPERTIES PUBLIC_HEADER "${SolCorpHeaders}")
target_compile_definitions(solcorplib PRIVATE
                            SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
                            SC_VERSION="${PROJECT_VERSION}")
target_compile_definitions(solcorplib PRIVATE FLECS_CPP_NO_AUTO_REGISTRATION)

if(MSVC)
  target_compile_options(solcorplib PRIVATE /WX /W4 /permissive- /wd4244)
else()
  target_compile_options(solcorplib PRIVATE -Werror -Wall -Wextra -Wpedantic)
endif()
target_compile_options(solcorplib PRIVATE ${LUA54_CFLAGS})

target_link_libraries(solcorplib ${SDL2_LINK_LIBRARIES})
target_link_libraries(solcorplib ${SDL2IMAGE_LINK_LIBRARIES})
target_link_libraries(solcorplib ${SDL2TTF_LINK_LIBRARIES})
target_link_libraries(solcorplib flecs::flecs_static)
target_link_libraries(solcorplib spdlog::spdlog)
target_link_libraries(solcorplib imgui)
target_link_libraries(solcorplib ${LUA54_LINK_LIBRARIES})

# Embedded textures
set(RES_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/../textures/construction.png)
set(RES_GEN  ${CMAKE_CURRENT_BINARY_DIR}/construction_png.cpp)
add_custom_command(
  OUTPUT  ${RES_GEN}
  COMMAND ${CMAKE_COMMAND}
    -DINPUT=${RES_SRC}
    -DOUTPUT=${RES_GEN}
    -DVARNAME=construction_png
    -P ${CMAKE_SOURCE_DIR}/cmake/embed_resource.cmake
  DEPENDS ${RES_SRC} ${CMAKE_SOURCE_DIR}/cmake/embed_resource.cmake
  VERBATIM
  COMMENT "Embedding textures/construction.png -> construction_png.cpp"
)

add_library(resources STATIC ${RES_GEN})
target_include_directories(resources PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(solcorplib resources)

# Executable
add_executable(solcorp main.cpp)

target_include_directories(solcorp PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(MSVC)
  target_compile_options(solcorp PRIVATE /WX /W4 /permissive-)
else()
  target_compile_options(solcorp PRIVATE -Werror -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(solcorp PRIVATE solcorplib)

if(NOT WIN32)
    # Find shared libs relative to the executable so the package is self-contained
    set_target_properties(solcorp PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

file(GLOB TEXTURES "${CMAKE_SOURCE_DIR}/textures/*.png")
file(COPY ${TEXTURES}
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/textures)

add_custom_target(run
    COMMAND solcorp
    DEPENDS solcorp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT Running SolCorp
)

if(NOT WIN32)
  add_custom_target(debug
      COMMAND gdb $<TARGET_FILE:solcorp>
      USES_TERMINAL
      DEPENDS solcorp
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT Debugging SolCorp
  )
endif()

if(UNIX AND NOT APPLE)
    find_program(PATCHELF patchelf)
    if(NOT PATCHELF)
        message(WARNING "patchelf not found - bundled .so files will keep Nix store RPATHs and won't be portable")
    endif()
endif()

install(TARGETS solcorp RUNTIME DESTINATION .)

if(WIN32)
    install(CODE "
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES \"$<TARGET_FILE:solcorp>\"
            RESOLVED_DEPENDENCIES_VAR _r_deps
            UNRESOLVED_DEPENDENCIES_VAR _u_deps
            DIRECTORIES \"$<TARGET_FILE_DIR:SDL2::SDL2>\"
            PRE_EXCLUDE_REGEXES \"api-ms-.*\" \"ext-ms-.*\"
            POST_EXCLUDE_REGEXES \".*[Ss]ystem32/.*\\\\.dll\"
        )
        foreach(dep \${_r_deps})
            file(INSTALL \"\${dep}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}\")
        endforeach()
        if(_u_deps)
            message(WARNING \"Unresolved runtime deps: \${_u_deps}\")
        endif()
    ")
elseif(UNIX AND NOT APPLE)
    install(CODE "
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES \"$<TARGET_FILE:solcorp>\"
            RESOLVED_DEPENDENCIES_VAR _r_deps
            UNRESOLVED_DEPENDENCIES_VAR _u_deps
            PRE_EXCLUDE_REGEXES
                \"^libGL\" \"^libEGL\" \"^libGLX\" \"^libOpenGL\"
                \"^libdrm\" \"^libgbm\" \"^libvulkan\"
                \"^ld-linux\" \"^libc\\\\.so\" \"^libm\\\\.so\"
                \"^libdl\\\\.so\" \"^libpthread\" \"^librt\\\\.so\"
                \"^libgcc_s\" \"^libstdc\"
        )
        foreach(dep \${_r_deps})
            get_filename_component(_dep_name \"\${dep}\" NAME)
            file(INSTALL \"\${dep}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}\"
                FOLLOW_SYMLINK_CHAIN)
            if(NOT \"${PATCHELF}\" STREQUAL \"\")
                execute_process(
                    COMMAND \"${PATCHELF}\" --set-rpath \"$ORIGIN\"
                        \"\${CMAKE_INSTALL_PREFIX}/\${_dep_name}\"
                    RESULT_VARIABLE _patchelf_result
                )
                if(_patchelf_result)
                    message(WARNING \"patchelf failed for \${_dep_name}\")
                endif()
            endif()
        endforeach()
        if(_u_deps)
            message(WARNING \"Unresolved runtime deps: \${_u_deps}\")
        endif()
    ")
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/mods
    DESTINATION .)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/fonts
    DESTINATION .)

install(FILES ${CMAKE_SOURCE_DIR}/config.lua
    DESTINATION .)
