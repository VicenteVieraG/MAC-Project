#include <bayesClassifier.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <cmath>
#include <ranges>
#include <utility>

#include "dataSet.hpp"

void BayesClassifier::train(){
    this->resetCounters();
    std::cout<<"--------------- Read Data ----------------"<<std::endl;
    this->dataSet.read("data/dataset.csv");
    this->dataSet.print();

    std::cout<<"--------------- Training --------------------"<<std::endl;
    const double sampleSize = std::floor(static_cast<double>(this->dataSet.records.size()) * 0.8);
    const auto sample = std::views::take(this->dataSet.records, sampleSize);

    // Counting and Bin Table
    for(auto& person : sample) this->partialFit(person);

    std::cout
        <<this->registerTotal<<'\n'
        <<this->caminanTotal<<'\n'
        <<this->conducenTotal<<'\n'
        <<this->caminanTotal/this->registerTotal<<'\n'
        <<this->conducenTotal/this->registerTotal<<std::endl;
    
    std::cout<<"---------------- Print Bins Table -------------------------"<<std::endl;
    constexpr std::array<const char*, 3> ageLabels{"joven", "adulto", "mayor"};
    constexpr std::array<const char*, 5> incomeLabels{"muyBajo", "bajo", "medio", "alto", "muyAlto"};

    std::cout << std::setw(12) << "AgeBins"
              << std::setw(12) << "IncomeBins"
              << std::setw(10) << "Camina"
              << std::setw(10) << "Conduce" << '\n'
              << "--------------------------------------------" << std::endl;

    for(std::size_t age = 0; age < ageLabels.size(); ++age){
        for(std::size_t income = 0; income < incomeLabels.size(); ++income){
            const auto& counts = sampleBins[age][income];
            std::cout << std::setw(12) << ageLabels[age]
                      << std::setw(12) << incomeLabels[income]
                      << std::setw(10) << counts[static_cast<int>(Category::camina)]
                      << std::setw(10) << counts[static_cast<int>(Category::conduce)] << std::endl;
        }
    }
}

void BayesClassifier::partialFit(const Person& person) {
    const auto [ageBin, incomeBin] = getBin({person.age, person.income});

    ++sampleBins
        [static_cast<std::size_t>(ageBin)]
        [static_cast<std::size_t>(incomeBin)]
        [static_cast<std::size_t>(person.category)];

    if(person.category == Category::camina) caminanTotal++;
    else conducenTotal++;

    registerTotal++;
}

std::pair<BinAge, BinIncome> BayesClassifier::getBin(const std::pair<double, double> input) const {
    const auto& [age, income] = input;
    BinAge ageBin;

    if(age < 30.0) ageBin = BinAge::joven;
    else if(age < 60.0) ageBin = BinAge::adulto;
    else ageBin = BinAge::mayor;

    BinIncome incomeBin;
    if(income < 20000.0) incomeBin = BinIncome::muyBajo;
    else if(income < 30000.0) incomeBin = BinIncome::bajo;
    else if(income < 40000.0) incomeBin = BinIncome::medio;
    else if(income < 50000.0) incomeBin = BinIncome::alto;
    else incomeBin = BinIncome::muyAlto;

    return {ageBin, incomeBin};
}

std::pair<double, double> BayesClassifier::suavizadoLaplace(const std::pair<BinAge, BinIncome> bins) const {
    const auto [ageBin, incomeBin] = bins;
    const auto counts = this->sampleBins
        [static_cast<std::size_t>(ageBin)]
        [static_cast<std::size_t>(incomeBin)];
    constexpr double alpha = 1.0;
    const double K = static_cast<double>(this->sampleBins.size() * this->sampleBins.front().size());
    const double caminan = counts[static_cast<std::size_t>(Category::camina)];
    const double conducen = counts[static_cast<std::size_t>(Category::conduce)];

    return std::make_pair(
        (caminan + alpha) / (this->caminanTotal + alpha * K),
        (conducen + alpha) / (this->conducenTotal + alpha * K)
    );
}

Category BayesClassifier::predict(const Person& input) {
    const auto [age, income, category] = input;
    const auto [ageBin, incomeBin] = this->getBin({age, income});
    this->pPrioriCaminan = this->caminanTotal / this->registerTotal;
    this->pPrioriConducen = this->conducenTotal / this->registerTotal;

    const auto [lhpCaminan, lhpConducen] = this->suavizadoLaplace({ageBin, incomeBin});

    const double scoreCaminan = this->pPrioriCaminan * lhpCaminan;
    const double scoreConducen = this->pPrioriConducen * lhpConducen;

    const double normalizationCaminan = scoreCaminan / (scoreCaminan + scoreConducen);
    const double normalizationConducen = scoreConducen / (scoreCaminan + scoreConducen);

    return normalizationCaminan >= normalizationConducen ? Category::camina : Category::conduce;
}

void BayesClassifier::resetCounters() noexcept {
    this->dataSet.records.clear();
    this->sampleBins = {};
    this->registerTotal = {};
    this->caminanTotal = {};
    this->conducenTotal = {};
    this->pPrioriCaminan = {};
    this->pPrioriConducen = {};
}