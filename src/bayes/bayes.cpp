#include <bayes.hpp>

#include <imgui.h>

#include "bayesClassifier.hpp"

void Bayes::draw(){
    static bool print = true; // Test
    static BayesClassifier bayesClassifier;

    // Test
    if(print){
        bayesClassifier.train();
        print = false;
    }

    ImGui::TextUnformatted("Welcome to Bayes! This is sample text for testing :)");
    
}