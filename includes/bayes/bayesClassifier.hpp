#pragma once

#include <utility>
#include <array>
#include <vector>

#include "dataSet.hpp"

class BayesClassifier {
    public:
        void train();
        void partialFit(const Person& person);
        Category predict(const Person& input);
        Person addPerson(const Person& person);
        auto getDataSet() const noexcept -> const DataSet& { return this->dataSet; };
        auto getTestRecords() const noexcept -> const std::vector<Person>& { return this->testRecords; };
        auto getSampleBins() const noexcept -> const auto& { return this->sampleBins; };
        auto getRegisterTotal() const noexcept -> double { return this->registerTotal; };
    private:
        double registerTotal{};
        double caminanTotal{};
        double conducenTotal{};
        double pPrioriCaminan{};
        double pPrioriConducen{};

        DataSet dataSet;
        // Original test subset, kept separate from people added for training.
        std::vector<Person> testRecords;
        // Access Indexes: [ageBin][incomeBin][category]
        std::array<std::array<std::array<int, 2>,5>,3> sampleBins{};
        // Turns a Person record into a Bin
        std::pair<BinAge, BinIncome> getBin(const std::pair<double, double> input) const;
        // Applies suavizado de Laplace for the respective bins of an input Person and returns thie likelihood probablities for <caminan, conducen>
        std::pair<double, double> suavizadoLaplace(const std::pair<BinAge, BinIncome> bins) const;
        void resetCounters() noexcept;
};
