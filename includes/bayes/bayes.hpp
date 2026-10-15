#pragma once

#include <optional>
#include <string>
#include "bayesClassifier.hpp"

class Bayes {
    public:
        void draw();

    private:
        void drawPrediction();
        void drawNewPerson();

        std::optional<BayesClassifier> bayesClassifier;
        std::string trainingError;
        Person predictionInput{25.0, 25000.0, Category::camina};
        Person newPersonInput{25.0, 25000.0, Category::camina};
        std::optional<Category> predictionResult;
        std::optional<Person> addedPerson;
};
