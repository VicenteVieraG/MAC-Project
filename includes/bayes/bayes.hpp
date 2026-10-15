#pragma once

#include <optional>
#include "bayesClassifier.hpp"

class Bayes {
    public: void draw();
    private: std::optional<BayesClassifier> bayesClassifier;
};