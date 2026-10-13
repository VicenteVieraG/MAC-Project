add_library(bayes STATIC
    bayes.cpp
    dataSet.cpp
    evaluationResult.cpp
    bayesClassifier.cpp
)
target_link_libraries(bayes PUBLIC rlimgui)