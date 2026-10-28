option(SDL_SHARED "Build SDL as a shared library" OFF)
option(SDL_STATIC "Build SDL as a static library" ON)
option(SDL_TEST_LIBRARY "Build the SDL test library" OFF)

# These optional X11 extensions must not prevent a desktop application build.
# Override cached SDL defaults too, so existing IDE build directories recover.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 QUIET)
    if(NOT X11_Xss_FOUND)
        message(STATUS "X11 screensaver development files not found; disabling SDL_X11_XSCRNSAVER")
        set(SDL_X11_XSCRNSAVER OFF)
    endif()
    if(NOT X11_Xtst_FOUND)
        message(STATUS "X11 XTest development files not found; disabling SDL_X11_XTEST")
        set(SDL_X11_XTEST OFF)
    endif()
endif()

add_subdirectory(sdl3 EXCLUDE_FROM_ALL)

# Dear ImGui does not provide a root CMakeLists.txt; build its sources directly.
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
)
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

add_library(implot STATIC
    implot/implot.cpp
    implot/implot_items.cpp
    implot/implot_demo.cpp
)
target_include_directories(implot PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/implot")
target_link_libraries(implot PRIVATE imgui)

add_library(imgui_sdl3 STATIC
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui_sdl3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")
target_link_libraries(imgui_sdl3
    PUBLIC imgui
    PUBLIC SDL3::SDL3
)
