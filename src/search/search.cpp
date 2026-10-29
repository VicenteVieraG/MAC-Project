#include <search.hpp>

#include <imgui.h>

void Search::draw() {
    ImGui::SeparatorText("Search");
    ImGui::TextWrapped("Explore shortest paths with Dijkstra and A* search.");
    ImGui::Spacing();

    if(ImGui::BeginTabBar("SearchViews")) {
        if(ImGui::BeginTabItem("Graph")) {
            ImGui::TextWrapped("Load a weighted graph and follow the search step by step.");
            ImGui::TextDisabled("Graph loading and visualization are not available yet.");
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Grid Editor")) {
            ImGui::TextWrapped("Design a map with obstacles, a starting point, and a goal.");
            ImGui::TextDisabled("Map editing is not available yet.");
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Comparison")) {
            ImGui::TextWrapped("Compare Dijkstra with Manhattan and Euclidean A* search on the same map.");
            ImGui::TextDisabled("Search results and comparisons are not available yet.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
