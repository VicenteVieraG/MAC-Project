add_library(bayes STATIC
    bayes.cpp
    bayesTables.cpp
    bayesPlots.cpp
    dataSet.cpp
    evaluationResult.cpp
    bayesClassifier.cpp
)
target_link_libraries(bayes
    PUBLIC imgui
    PUBLIC implot
)
