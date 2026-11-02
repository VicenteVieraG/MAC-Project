#include <search.hpp>

#include <imgui.h>

void Search::draw() {
    ImGui::SeparatorText("Search");
    ImGui::TextWrapped("Explore shortest paths with Dijkstra and A* search.");
    ImGui::Spacing();

    if(ImGui::BeginTabBar("SearchViews")) {
        if(ImGui::BeginTabItem("Grid")) {
            this->gridView.draw();
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Graph")) {
            this->graphView.draw();
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Benchmarking")) {
            this->benchmarkView.draw();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
