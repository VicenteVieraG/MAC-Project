add_library(search_core STATIC
    graph.cpp
    dijkstra.cpp
    heuristics.cpp
    heap.cpp
    graphReader.cpp
)

add_library(search STATIC search.cpp)
target_link_libraries(search
    PUBLIC search_core
    PRIVATE imgui
)
