add_library(search_core STATIC pathfinding.cpp)

add_library(search STATIC search.cpp)
target_link_libraries(search
    PUBLIC search_core
    PRIVATE imgui
)
