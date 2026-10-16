#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include "bayesClassifier.hpp"
#include "evaluationResult.hpp"

class Bayes {
    public:
        void draw();
    private:
        void drawContent();
        void drawDataView();
        void drawPlotsView();
        void drawScatterPlot(float height);
        void drawConfusionMatrix();
        void refreshEvaluation();
        void drawBinsTable(float height);
        void drawDataSetTable(float height);
        void drawSidebar();
        void drawPrediction();
        void drawNewPerson();

        std::optional<BayesClassifier> bayesClassifier;
        EvaluationResult evaluationResult;
        std::string trainingError;
        Person predictionInput{25.0, 25000.0, Category::camina};
        Person newPersonInput{25.0, 25000.0, Category::camina};
        std::optional<Category> predictionResult;
        std::optional<Person> addedPerson;
        std::size_t scatterRecordCount{};
};
