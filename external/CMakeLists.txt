add_subdirectory(raylib)

# Dear ImGui does not provide a root CMakeLists.txt; build its sources directly.
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
)
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

add_library(rlimgui STATIC rlimgui/rlImGui.cpp)
target_include_directories(rlimgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/rlimgui")
target_link_libraries(rlimgui
    PUBLIC imgui
    PUBLIC raylib
)
