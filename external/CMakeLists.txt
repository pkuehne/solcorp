list(APPEND imgui_sources
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_sdlrenderer2.cpp
    imgui/misc/cpp/imgui_stdlib.cpp
)

list(APPEND imgui_headers
    imgui/imgui.h
    imgui/imgui_internal.h
    imgui/imgui_rectpack.h
    imgui/imgui_textedit.h
    imgui/imgui_truetype.h
    imgui/backends/imgui_impl_sdl2.h
    imgui/backends/imgui_impl_sdlrenderer2.h
    imgui/misc/cpp/imgui_stdlib.cpp
)

# Add SDL2 dependencies
if(WIN32)
  find_package(SDL2 REQUIRED CONFIG)
  find_package(SDL2_image REQUIRED CONFIG)
  set(SDL2_INCLUDE_DIRS "")
  set(SDL2_LINK_LIBRARIES SDL2::SDL2)
  set(SDL2IMAGE_INCLUDE_DIRS "")
  set(SDL2IMAGE_LINK_LIBRARIES SDL2_image::SDL2_image)
else()
  include(FindPkgConfig)
  pkg_search_module(SDL2 REQUIRED sdl2)
  pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
  pkg_search_module(SDL2TTF REQUIRED SDL2_ttf>=2.0.14)
endif()

add_library(imgui STATIC "${imgui_sources}")
target_include_directories(imgui PUBLIC ./imgui/)
target_include_directories(imgui PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(imgui PUBLIC ${SDL2IMAGE_INCLUDE_DIRS})

target_link_libraries(imgui ${SDL2_LINK_LIBRARIES})
target_link_libraries(imgui ${SDL2IMAGE_LINK_LIBRARIES})

# set_target_properties(imgui PROPERTIES PUBLIC_HEADER "${imgui_headers}")
