add_library(appLayout STATIC appLayout.cpp)
target_link_libraries(appLayout
    PUBLIC navBar
)