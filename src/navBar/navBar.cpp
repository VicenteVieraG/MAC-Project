#include <navBar.hpp>

#include <iostream>

#include <imgui.h>

NavBar::NavBar(){
    // Set initial screen
    this->currentScreen = Screen::Home;
}

void NavBar::draw() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::MenuItem("Home", nullptr, this->currentScreen == Screen::Home)) this->currentScreen = Screen::Home;

        if (ImGui::MenuItem("Bayes", nullptr, this->currentScreen == Screen::Bayes)) this->currentScreen = Screen::Bayes;
        

        ImGui::EndMainMenuBar();
    }
}

Screen NavBar::getCurrentScreen() const noexcept {
    return this->currentScreen;
}