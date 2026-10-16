#include "evaluationResult.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>

void EvaluationResult::evaluate(BayesClassifier& classifier, const std::vector<Person>& testRecords) {
    evaluated = false;
    evaluatedSampleCount = 0;
    VP = 0;
    VN = 0;
    FP = 0;
    FN = 0;
    sensibilidad.reset();
    especificidad.reset();

    if(classifier.getRegisterTotal() == 0.0)
        throw std::logic_error("Cannot evaluate an untrained classifier.");

    for(const Person& person : testRecords){
        Category real = person.category;
        Category predicted = classifier.predict(person);

        //VP: Verdadero Positivo, VN: Verdadero Negativo, FP: Falso Positivo, FN: Falso Negativo

        if(real == Category::camina && predicted == Category::camina) VP++;
        else if(real == Category::conduce && predicted == Category::conduce) VN++;
        else if(real == Category::conduce && predicted == Category::camina) FP++;
        else if(real == Category::camina && predicted == Category::conduce) FN++;
        ++evaluatedSampleCount;
    }

    if (VP + FN > 0) sensibilidad = static_cast<double>(VP) / (VP + FN);
    if (VN + FP > 0) especificidad = static_cast<double>(VN) / (VN + FP);
    evaluated = true;
}

void EvaluationResult::print() const {
    if(!evaluated){
        std::cerr<<"Evaluacion no disponible."<<std::endl;
        return;
    }
    if(evaluatedSampleCount == 0){
        std::cerr<<"No hay muestras de prueba disponibles."<<std::endl;
        return;
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

    if (sensibilidad.has_value()) {

        std::cout << "\nSensibilidad: "
                  << *sensibilidad * 100 << "%";

        std::cout << "\nComplemento sensibilidad: "
                  << (1.0 - *sensibilidad) * 100 << "%";
    }
    else {
        std::cout << "\nSensibilidad: No definida";
        std::cout << "\nComplemento sensibilidad: No definido";
    }

    if (especificidad.has_value()) {

        std::cout << "\nEspecificidad: "
                  << *especificidad * 100 << "%";

        std::cout << "\nComplemento especificidad: "
                  << (1.0 - *especificidad) * 100 << "%";
    }
    else {
        std::cout << "\nEspecificidad: No definida";
        std::cout << "\nComplemento especificidad: No definido";
    }

    std::cout << '\n';
}
