#pragma once

#include <utility>
#include <array>

#include "dataSet.hpp"

class BayesClassifier {
    public:
        void train();
        Category predict(const Person input);
        void predictProba();
    private:
        double registerTotal;
        double caminanTotal;
        double conducenTotal;
        double pPrioriCaminan;
        double pPrioriConducen;

        DataSet dataSet;
        // Access Indexes: [ageBin][incomeBin][category]
        std::array<std::array<std::array<int, 2>,5>,3> sampleBins{};
        // Turns a Person record into a Bin
        std::pair<BinAge, BinIncome> getBin(const std::pair<int, int> input) const;
        // Applies suavizado de Laplace for the respective bins of an input Person and returns thie likelihood probablities for <caminan, conducen>
        std::pair<double, double> suavizadoLaplace(const std::pair<BinAge, BinIncome> bins) const;
};