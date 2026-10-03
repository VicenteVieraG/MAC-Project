add_library(window STATIC window.cpp)
target_link_libraries(window
    PRIVATE imguiConfig
    PUBLIC rlimgui
)