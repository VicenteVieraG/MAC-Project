#pragma once

#include <utility>

#include "dataSet.hpp"

class BayesClassifier {
    public:
        void predict();
        void train(); // Make private for release
    private:
        double registerTotal;
        double countCaminan;
        double countConducen;

        void predictProba();
        std::pair<double, double> suavizadoLaplace(const int count) const;
};