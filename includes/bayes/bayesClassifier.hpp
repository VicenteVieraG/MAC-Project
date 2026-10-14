#pragma once

#include <utility>
#include <array>

#include "dataSet.hpp"

class BayesClassifier {
    public:
        std::pair<double, double> predict(const Person input);
        void train(); // Make private for release
    private:
        double registerTotal;
        double caminanTotal;
        double conducenTotal;
        double pPrioriCaminan;
        double pPrioriConducen;


        // [incomeBin][ageBin][category]
        std::array<std::array<std::array<int, 2>,3>,5> sampleBins{};
        std::pair<BinAge, BinIncome> getBin(const std::pair<int, int> input) const;
        void predictProba();
        std::pair<double, double> suavizadoLaplace(const std::pair<BinAge, BinIncome> count) const;
};