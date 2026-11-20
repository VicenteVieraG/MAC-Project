#include <coloringView.hpp>

#include <algorithm>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <raylib.h>

namespace coloring {
namespace {
constexpr const char* exampleNames[]{"Activity 17", "Odd cycle", "Complete bipartite", "Irregular tree"};
constexpr const char* exampleFiles[]{"activity-17.txt", "odd-cycle.txt", "bipartite.txt", "irregular-tree.txt"};
}

void ColoringView::clearRuns() {
    runs = {};
    selected = -1;
    exportMessage.clear();
}

void ColoringView::load(const std::string& text, const std::string& name) {
    auto next = parseGraph(text);
    graph = std::move(next);
    draft = text;
    graphName = name;
    inputError.clear();
    exact.reset();
    exactError.clear();
    clearRuns();
}

void ColoringView::loadExample(int index) {
    try {
        auto path = std::filesystem::path(GetApplicationDirectory()) / "data" / "coloring" / exampleFiles[index];
        load(readGraphFile(path), exampleNames[index]);
    } catch (const std::exception& error) { inputError = error.what(); }
}

void ColoringView::draw() {
    if (!initialized) {
        loadExample(0);
        initialized = true;
    }
    ImGui::PushID("Phase3");
    if (!graph.labels.empty())
        ImGui::TextWrapped("%s   /   %zu vertices, %zu edges", graphName.c_str(), graph.labels.size(), graph.edges.size());
    else ImGui::TextWrapped("Open Graph input to load a graph. %s", inputError.c_str());

    if (ImGui::BeginTabBar("ColoringViews")) {
        if (ImGui::BeginTabItem("Coloring")) {
            if (!graph.labels.empty()) drawWorkbench();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Graph input")) {
            drawInput();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Comparison")) {
            if (!graph.labels.empty()) drawComparison();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopID();
}

void ColoringView::drawInput() {
    if (ImGui::BeginCombo("Load example", "Choose a graph")) {
        for (int i = 0; i < 4; ++i)
            if (ImGui::Selectable(exampleNames[i])) loadExample(i);
        ImGui::EndCombo();
    }
    ImGui::SeparatorText("Import a text file");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##filePath", "Path to graph.txt", &filePath);
    if (ImGui::Button("Load file")) {
        try { load(readGraphFile(filePath), std::filesystem::path(filePath).filename().string()); }
        catch (const std::exception& error) { inputError = error.what(); }
    }
    ImGui::SeparatorText("Edge list");
    ImGui::TextWrapped("Enter n m, then n unique vertex IDs on the next line. Add one pair of endpoints per edge.");
    ImGui::InputTextMultiline("##graphText", &draft, ImVec2(-1, ImGui::GetTextLineHeight() * 7.5f));
    if (ImGui::Button("Validate and load")) {
        try { load(draft, "Custom graph"); }
        catch (const std::exception& error) { inputError = error.what(); }
    }
    if (!inputError.empty()) ImGui::TextWrapped("Could not load graph. %s", inputError.c_str());
}

void ColoringView::drawWorkbench() {
    const auto available = ImGui::GetContentRegionAvail();
    if (available.y <= 0 || available.x <= 0) return;
    const bool stacked = available.x < 580;
    const float controlsWidth = stacked ? available.x : std::min(310.0f, available.x * 0.42f);
    if (ImGui::BeginChild("Controls", ImVec2(controlsWidth, stacked ? available.y * 0.48f : 0), ImGuiChildFlags_Borders))
        drawControls();
    ImGui::EndChild();
    if (!stacked) ImGui::SameLine();
    if (ImGui::BeginChild("GraphPanel", ImVec2(0, 0), ImGuiChildFlags_Borders)) drawDiagram();
    ImGui::EndChild();
}

void ColoringView::drawControls() {
    ImGui::SeparatorText("Palette");
    ImGui::TextUnformatted("Available colors (k)");
    ImGui::SetNextItemWidth(-1);
    bool edited = ImGui::InputInt("##k", &k);
    ImGui::TextUnformatted("Names in priority order");
    ImGui::SetNextItemWidth(-1);
    edited |= ImGui::InputText("##palette", &colorNames);
    if (edited) {
        clearRuns();
        try {
            palette = parsePalette(k, colorNames);
            paletteError.clear();
        } catch (const std::exception& error) {
            palette.clear();
            paletteError = error.what();
        }
    }
    if (!paletteError.empty()) ImGui::TextWrapped("%s", paletteError.c_str());
    ImGui::SeparatorText("Coloring strategy");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##Algorithm", &algorithm, algorithmNames, 3);
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##Mode", &mode, modeNames, 4);
    ImGui::BeginDisabled(palette.empty());
    if (ImGui::Button("Run coloring", ImVec2(-1, 0))) run(algorithm, mode);
    ImGui::EndDisabled();
    drawExact();
}

void ColoringView::calculateExact() {
    try { exact = analyze(graph); exactError.clear(); }
    catch (const std::exception& error) { exactError = error.what(); }
}

void ColoringView::drawExact() {
    ImGui::SeparatorText("Exact reference");
    if (graph.labels.size() > maxExactVertices) {
        ImGui::TextWrapped("Exact analysis supports up to 10 vertices.");
        return;
    }
    if (ImGui::Button("Calculate polynomial and chi", ImVec2(-1, 0))) calculateExact();
    if (!exactError.empty()) ImGui::TextWrapped("%s", exactError.c_str());
    if (!exact) return;
    ImGui::TextWrapped("P(G,q) = %s", exact->formula().c_str());
    ImGui::Text("Chromatic number = %d", exact->chromaticNumber);
    if (k >= 1 && k <= maxColors) ImGui::Text("P(G,%d) = %lld", k, static_cast<long long>(exact->evaluate(k)));
    ImGui::Text("%d calls, %.3f ms", exact->calls, exact->milliseconds);
    if (!exact->trace.empty() && ImGui::TreeNode("Deletion-contraction trace")) {
        ImGui::TextWrapped("Indices follow the vertex list. Each contraction merges the second endpoint into the first and closes the index gap.");
        ImGui::TextUnformatted(exact->trace.c_str());
        ImGui::TreePop();
    }
}

void ColoringView::run(int a, int m) {
    std::array<double, 5> times;
    Run result;
    for (auto& t : times) {
        result = color(graph, k, static_cast<Algorithm>(a), static_cast<Mode>(m));
        t = result.milliseconds;
    }
    std::sort(times.begin(), times.end());
    result.milliseconds = times[2];
    selected = a * 4 + m;
    runs[selected] = std::move(result);
    exportMessage.clear();
}

std::string ColoringView::orderText(const Run& result) const {
    std::string order;
    for (int v : result.order) {
        if (!order.empty()) order += " > ";
        order += graph.labels[v];
    }
    return order;
}

std::string ColoringView::blockersText(const Run& result) const {
    std::string blockers;
    if (!result.success()) {
        for (int v : graph.neighbors[result.failedVertex]) {
            if (result.colors[v] < 0) continue;
            if (!blockers.empty()) blockers += ", ";
            blockers += graph.labels[v] + "=" + palette[result.colors[v]];
        }
    }
    return blockers;
}
} // namespace coloring
