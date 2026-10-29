add_library(window STATIC window.cpp)
target_link_libraries(window
    PRIVATE imguiConfig
    PRIVATE implot
    PRIVATE rlimgui
    PRIVATE imgui
    PRIVATE raylib
)
