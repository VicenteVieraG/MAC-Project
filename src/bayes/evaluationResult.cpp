#include "evaluationResult.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>

void EvaluationResult::evaluate(BayesClassifier& classifier) {

    int VP = 0;
    int VN = 0;
    int FP = 0;
    int FN = 0;

    const auto& dataSet = classifier.getDataSet();

    const std::size_t sampleSize = static_cast<std::size_t>(
        std::floor(dataSet.records.size() * 0.8)
    );

    // Recorrer el 20% restante
    for (std::size_t i = sampleSize;
         i < dataSet.records.size(); i++) {

        const Person& person = dataSet.records[i];
        Category real = person.category;
        Category predicted = classifier.predict(person);

        //VP: Verdadero Positivo, VN: Verdadero Negativo, FP: Falso Positivo, FN: Falso Negativo

        if (real == Category::camina &&
            predicted == Category::camina) {
            VP++;
        }
        else if (real == Category::conduce &&
                 predicted == Category::conduce) {
            VN++;
        }
        else if (real == Category::conduce &&
                 predicted == Category::camina) {
            FP++;
        }
        else if (real == Category::camina &&
                 predicted == Category::conduce) {
            FN++;
        }
    }

    std::cout << "\n========== MATRIZ DE CONFUSION ==========\n";

    std::cout << std::setw(18) << " "
            << std::setw(12) << "Pred. Camina"
            << std::setw(16) << "Pred. Conduce" << '\n';

    std::cout << std::setw(18) << "Real Camina"
            << std::setw(12) << VP
            << std::setw(16) << FN << '\n';

    std::cout << std::setw(18) << "Real Conduce"
            << std::setw(12) << FP
            << std::setw(16) << VN << '\n';

    std::cout << "=========================================\n";

    std::cout << std::fixed << std::setprecision(2);

    if (VP + FN > 0) {

        double sensibilidad =
            static_cast<double>(VP) / (VP + FN);

        std::cout << "\nSensibilidad: "
                  << sensibilidad * 100 << "%";

        std::cout << "\nComplemento sensibilidad: "
                  << (1.0 - sensibilidad) * 100 << "%";
    }
    else {
        std::cout << "\nSensibilidad: No definida";
        std::cout << "\nComplemento sensibilidad: No definido";
    }

    if (VN + FP > 0) {

        double especificidad =
            static_cast<double>(VN) / (VN + FP);

        std::cout << "\nEspecificidad: "
                  << especificidad * 100 << "%";

        std::cout << "\nComplemento especificidad: "
                  << (1.0 - especificidad) * 100 << "%";
    }
    else {
        std::cout << "\nEspecificidad: No definida";
        std::cout << "\nComplemento especificidad: No definido";
    }

    std::cout << '\n';
}