#include <bayesClassifier.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <cmath>
#include <ranges>
#include <span>
#include <utility>

#include "dataSet.hpp"

void BayesClassifier::train(){
    DataSet dataSet;

    std::cout<<"--------------- Read Data ----------------"<<std::endl;
    dataSet.read("data/dataset.csv");
    dataSet.print();

    std::cout<<"--------------- Probabilidad a Priori --------------------"<<std::endl;
    const double sampleSize = std::floor(static_cast<double>(dataSet.records.size()) * 0.8);
    const auto sample = std::views::take(dataSet.records, sampleSize);

    for(auto& record : sample)
        if(record.category == Category::camina) this->countCaminan++;
        else this->countConducen++;
    this->registerTotal = static_cast<double>(sample.size());

    // Priori Probability
    const double pPrioriCaminan = this->countCaminan/this->registerTotal;
    const double pPrioriConducen = this->countConducen/this->registerTotal;

    std::cout
        <<registerTotal<<'\n'
        <<countCaminan<<'\n'
        <<countConducen<<'\n'
        <<pPrioriCaminan<<'\n'
        <<pPrioriConducen<<std::endl;
    
    std::cout<<"---------------- Bins -------------------------"<<std::endl;

    std::array // [ageBin][incomeBin][category]
        <std::array
            <std::array
                <int, 2>
            ,3>
        ,5>
    sampleBins{};

    for(auto& [age, income, category] : sample){
        BinAge ageBin;
        if(age < 30) ageBin = BinAge::joven;
        else if(age < 60) ageBin = BinAge::adulto;
        else ageBin = BinAge::mayor;

        BinIncome incomeBin;
        if(income < 20000) incomeBin = BinIncome::muyBajo;
        else if(income < 30000) incomeBin = BinIncome::bajo;
        else if(income < 40000) incomeBin = BinIncome::medio;
        else if(income < 50000) incomeBin = BinIncome::alto;
        else incomeBin = BinIncome::muyAlto;

        sampleBins[static_cast<int>(incomeBin)][static_cast<int>(ageBin)][static_cast<int>(category)]++;
    }

    // Print bins
    constexpr std::array<const char*, 3> ageLabels{"joven", "adulto", "mayor"};
    constexpr std::array<const char*, 5> incomeLabels{"muyBajo", "bajo", "medio", "alto", "muyAlto"};

    std::cout << std::setw(12) << "AgeBins"
              << std::setw(12) << "IncomeBins"
              << std::setw(10) << "Camina"
              << std::setw(10) << "Conduce" << '\n'
              << "--------------------------------------------" << '\n';

    for(std::size_t age = 0; age < ageLabels.size(); ++age){
        for(std::size_t income = 0; income < incomeLabels.size(); ++income){
            const auto& counts = sampleBins[income][age];
            std::cout << std::setw(12) << ageLabels[age]
                      << std::setw(12) << incomeLabels[income]
                      << std::setw(10) << counts[static_cast<int>(Category::camina)]
                      << std::setw(10) << counts[static_cast<int>(Category::conduce)] << '\n';
        }
    }
}

std::pair<double, double> BayesClassifier::suavizadoLaplace(const int count) const {
    const double k = 3.0 * 5.0; // 3 age bins * 5 income bins
    const double alpha = 1.0;

    return std::make_pair(
        static_cast<double>(count + alpha) / (this->countCaminan + alpha * k),
        static_cast<double>(count + alpha) / (this->countConducen + alpha * k)
    );
}
