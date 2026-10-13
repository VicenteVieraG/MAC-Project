#pragma once

#include <utility>
#include <array>

#include "dataSet.hpp"

class BayesClassifier {
    public:
        void predict();
        void train(); // Make private for release
    private:
        double registerTotal;
        double caminanTotal;
        double conducenTotal;
        double pPrioriCaminan;
        double pPrioriConducen;


        // [ageBin][incomeBin][category]
        std::array<std::array<std::array<int, 2>,3>,5> sampleBins{};

        void predictProba();
        std::pair<double, double> suavizadoLaplace(const std::pair<int, int> count) const;
};