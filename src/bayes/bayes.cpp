#include <bayes.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <string>

#include <imgui.h>
#include <implot.h>

#include "bayesClassifier.hpp"

namespace {
    std::string categoryLabel(Category category) {
        return category == Category::camina ? "Camina (walks)" : "Conduce (drives)";
    }

    std::string validatePerson(const Person& person) {
        const auto& [age, income, _] = person;

        if(!std::isfinite(age) || age <= 0.0)
            return "Enter an age greater than zero.";
        if(!std::isfinite(income) || income < 0.0)
            return "Enter an income of zero or more.";
        return {};
    }

    bool drawPersonInputs(Person& person) {
        bool changed = false;
        if(ImGui::BeginTable("PersonInputs", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Age (years)");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Income");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1.0f);
            changed |= ImGui::InputDouble("##age", &person.age, 0.0, 0.0, "%.15g");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1.0f);
            changed |= ImGui::InputDouble("##income", &person.income, 0.0, 0.0, "%.15g");
            ImGui::EndTable();
        }
        return changed;
    }

    void drawHint(const std::string& message) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", message.c_str());
        ImGui::PopStyleColor();
    }
}

void Bayes::draw(){
    if(!this->bayesClassifier.has_value()) {
        this->bayesClassifier.emplace();
        try {
            this->bayesClassifier->train();
        } catch(const std::exception& error) {
            this->trainingError = std::string("Could not load training data: ") + error.what();
            this->bayesClassifier.emplace();
        }
    }

    const ImVec2 available = ImGui::GetContentRegionAvail();
    if(const auto [x, y] = available; x <= 0.0f || y <= 0.0f) return;

    const float sidebarWidth = std::min(available.x, 20.0f * ImGui::GetFontSize());
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available.x - sidebarWidth);
    if(ImGui::BeginChild("BayesSidebar", ImVec2(sidebarWidth, available.y), ImGuiChildFlags_Borders)) {
        ImGui::TextUnformatted("Bayes");
        if(!this->trainingError.empty())
            ImGui::TextWrapped("%s", this->trainingError.c_str());
        const double trainingSampleCount = this->bayesClassifier->getRegisterTotal();
        ImGui::TextDisabled("Training samples: %.0f", trainingSampleCount);
        if(trainingSampleCount == 0.0)
            ImGui::TextWrapped("No training samples loaded. Add a person below to start.");

        this->drawPrediction();
        ImGui::Spacing();
        this->drawNewPerson();
    }
    ImGui::EndChild();
}

void Bayes::drawPrediction() {
    ImGui::PushID("prediction");
    ImGui::SeparatorText("Predict");

    if(drawPersonInputs(this->predictionInput)) this->predictionResult.reset();
    const std::string error = validatePerson(this->predictionInput);

    ImGui::BeginDisabled(!error.empty() || this->bayesClassifier->getRegisterTotal() == 0.0);
    if(ImGui::Button("Predict", ImVec2(-1.0f, 0.0f)))
        this->predictionResult = this->bayesClassifier->predict(this->predictionInput);
    ImGui::EndDisabled();

    if(!error.empty()) ImGui::TextWrapped("%s", error.c_str());
    else if(this->predictionResult)
        ImGui::TextWrapped("Prediction: %s", categoryLabel(*this->predictionResult).c_str());
    else drawHint("The predicted category will appear here.");

    ImGui::PopID();
}

void Bayes::drawNewPerson() {
    ImGui::PushID("newPerson");
    ImGui::SeparatorText("Add person");

    bool changed = drawPersonInputs(this->newPersonInput);
    ImGui::TextUnformatted("Known category");
    ImGui::SetNextItemWidth(-1.0f);
    if(ImGui::BeginCombo("##category", categoryLabel(this->newPersonInput.category).c_str())) {
        constexpr std::array categories{Category::camina, Category::conduce};
        for(const Category category : categories) {
            const bool selected = this->newPersonInput.category == category;
            if(ImGui::Selectable(categoryLabel(category).c_str(), selected)) {
                this->newPersonInput.category = category;
                changed = true;
            }
            if(selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if(changed) this->addedPerson.reset();
    const std::string error = validatePerson(this->newPersonInput);

    ImGui::BeginDisabled(!error.empty());
    if(ImGui::Button("Add person", ImVec2(-1.0f, 0.0f))) {
        this->bayesClassifier->partialFit(this->newPersonInput);
        this->addedPerson = this->newPersonInput;
        // Keep an existing prediction in sync with the updated model.
        if(this->predictionResult)
            this->predictionResult = this->bayesClassifier->predict(this->predictionInput);
    }
    ImGui::EndDisabled();

    if(!error.empty()) ImGui::TextWrapped("%s", error.c_str());
    else if(this->addedPerson) {
        ImGui::TextUnformatted("Person added to the model.");
        ImGui::TextWrapped("Age: %.15g | Income: %.15g | %s",
            this->addedPerson->age, this->addedPerson->income, categoryLabel(this->addedPerson->category).c_str());
    } else drawHint("The added person's details will appear here.");
    drawHint("Added people are used for this session only.");

    ImGui::PopID();
}
