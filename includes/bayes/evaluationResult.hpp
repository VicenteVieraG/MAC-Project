#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "bayesClassifier.hpp"

class EvaluationResult {
public:
    void evaluate(BayesClassifier& classifier, const std::vector<Person>& testRecords);
    void print() const;
    auto hasEvaluated() const noexcept -> bool { return this->evaluated; };
    auto getEvaluatedSampleCount() const noexcept -> std::size_t { return this->evaluatedSampleCount; };
    auto getVP() const noexcept -> int { return this->VP; };
    auto getVN() const noexcept -> int { return this->VN; };
    auto getFP() const noexcept -> int { return this->FP; };
    auto getFN() const noexcept -> int { return this->FN; };
    auto getSensibilidad() const -> std::optional<double> { return this->sensibilidad; };
    auto getEspecificidad() const -> std::optional<double> { return this->especificidad; };

private:
    bool evaluated = false;
    std::size_t evaluatedSampleCount = 0;
    int VP = 0;
    int VN = 0;
    int FP = 0;
    int FN = 0;
    std::optional<double> sensibilidad;
    std::optional<double> especificidad;
};
