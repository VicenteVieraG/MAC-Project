add_library(bayes STATIC
    bayes.cpp
    bayesTables.cpp
    dataSet.cpp
    evaluationResult.cpp
    bayesClassifier.cpp
)
target_link_libraries(bayes
    PUBLIC rlimgui
    PUBLIC implot
)
