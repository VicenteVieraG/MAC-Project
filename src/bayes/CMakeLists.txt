add_library(bayes_core STATIC
    dataSet.cpp
    evaluationResult.cpp
    bayesClassifier.cpp
)

add_library(bayes STATIC
    bayes.cpp
    bayesTables.cpp
    bayesPlots.cpp
)
target_link_libraries(bayes
    PUBLIC bayes_core
    PRIVATE imgui
    PRIVATE implot
)
