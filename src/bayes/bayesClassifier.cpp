#include <bayesClassifier.hpp>

#include <iostream>
#include <vector>
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

}