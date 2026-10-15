#pragma once

#include <utility>
#include <array>

#include "dataSet.hpp"

class BayesClassifier {
    public:
        void train();
        void partialFit(const Person& person);
        Category predict(const Person& input);
        auto getDataSet() const noexcept -> auto { return this->dataSet; };
        auto getSampleBins() const noexcept -> auto { return this->sampleBins; };
        auto getRegisterTotal() const noexcept -> double { return this->registerTotal; };
    private:
        double registerTotal{};
        double caminanTotal{};
        double conducenTotal{};
        double pPrioriCaminan{};
        double pPrioriConducen{};

        DataSet dataSet;
        // Access Indexes: [ageBin][incomeBin][category]
        std::array<std::array<std::array<int, 2>,5>,3> sampleBins{};
        // Turns a Person record into a Bin
        std::pair<BinAge, BinIncome> getBin(const std::pair<double, double> input) const;
        // Applies suavizado de Laplace for the respective bins of an input Person and returns thie likelihood probablities for <caminan, conducen>
        std::pair<double, double> suavizadoLaplace(const std::pair<BinAge, BinIncome> bins) const;
        void resetCounters() noexcept;
};