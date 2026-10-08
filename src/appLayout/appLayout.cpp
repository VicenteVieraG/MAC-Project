#include <appLayout.hpp>
#include <imgui.h>
#include <navBar.hpp>

void AppLayout::draw(){
    this->navBar.draw();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings;

    if(ImGui::Begin("Workspace", nullptr, flags)) {
        switch (navBar.getCurrentScreen()) {
            case Screen::Home: home.draw(); break;
            // case Screen::Bayes: bayes.draw(); break;
        }
    }
    ImGui::End();
}
