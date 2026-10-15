#include <bayes.hpp>

#include <imgui.h>

#include "bayesClassifier.hpp"

void Bayes::draw(){
    if(!this->bayesClassifier.has_value()) {
        this->bayesClassifier.emplace();
        this->bayesClassifier->train();
    }

    ImGui::TextUnformatted("Welcome to Bayes!");

    if(ImGui::Button("Retrain classifier")) this->bayesClassifier->train();
}