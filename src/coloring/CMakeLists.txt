add_library(coloring_core STATIC coloringGraph.cpp chromaticPolynomial.cpp coloringRun.cpp)
add_library(coloring STATIC coloringView.cpp coloringComparison.cpp coloringDiagram.cpp)
target_link_libraries(coloring PUBLIC coloring_core PRIVATE imgui raylib)
