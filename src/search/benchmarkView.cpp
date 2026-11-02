#include <benchmarkView.hpp>

#include <graphReader.hpp>
#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

namespace {
    const char* heuristicName(Heuristics::Type type) {
        switch(type) {
            case Heuristics::Type::None: return "Dijkstra";
            case Heuristics::Type::Euclidean: return "Euclidean";
            case Heuristics::Type::Manhattan: return "Manhattan";
        }
        return "Unknown";
    }
}

void BenchmarkView::draw() {
    if(ImGui::BeginChild("BenchmarkView", ImVec2(0, 0))) {
        ImGui::TextWrapped("Compare Dijkstra, Euclidean and Manhattan on the same graph and endpoints.");
        this->drawControls();
        this->drawResults();
    }
    ImGui::EndChild();
}

void BenchmarkView::drawControls() {
    ImGui::SeparatorText("Load graph");
    ImGui::SetNextItemWidth(-1);
    const bool submitted = ImGui::InputTextWithHint("##benchmarkPath", "Graph file path", this->filePath.data(),
        this->filePath.size(), ImGuiInputTextFlags_EnterReturnsTrue);
    if(ImGui::Button("Load graph") || submitted) this->loadFile(this->filePath.data());
    ImGui::SameLine();
    if(ImGui::Button("Browse...")) {
        if(this->directoryPath[0] == '\0') {
            std::error_code error;
            auto directory = std::filesystem::current_path(error);
            if(error) directory = ".";
            std::snprintf(this->directoryPath.data(), this->directoryPath.size(), "%s", directory.string().c_str());
        }
        this->refreshDirectory = true;
        ImGui::OpenPopup("Choose benchmark graph");
    }
    this->drawFilePicker();

    if(ImGui::TreeNode("File format")) {
        ImGui::TextWrapped("Square adjacency matrix with comma-separated weights inside | on each row. INF means no edge; 0 is a valid weight. Weights must be nonnegative.");
        ImGui::TextUnformatted("| 0, 4, INF |\n| INF, 0, 2 |\n| INF, INF, 0 |");
        ImGui::TreePop();
    }

    ImGui::SeparatorText("Comparison setup");
    if(this->benchmark.loaded()) {
        ImGui::TextWrapped("Loaded: %s", this->loadedName.c_str());
        ImGui::Text("%zu nodes | Node IDs: 0 to %zu", this->benchmark.nodeCount(), this->benchmark.nodeCount() - 1);
    } else {
        ImGui::TextDisabled("Load a graph to enable the comparison.");
    }

    ImGui::BeginDisabled(!this->benchmark.loaded());
    int start = static_cast<int>(this->benchmark.start());
    int goal = static_cast<int>(this->benchmark.goal());
    const int maximum = this->benchmark.loaded() ? static_cast<int>(this->benchmark.nodeCount() - 1) : 0;
    ImGui::SetNextItemWidth(160.0f);
    if(ImGui::InputInt("Start node", &start)) {
        this->benchmark.setStart(static_cast<std::size_t>(std::clamp(start, 0, maximum)));
        this->errorMessage.clear();
    }
    ImGui::SetNextItemWidth(160.0f);
    if(ImGui::InputInt("Goal node", &goal)) {
        this->benchmark.setGoal(static_cast<std::size_t>(std::clamp(goal, 0, maximum)));
        this->errorMessage.clear();
    }
    if(ImGui::Button("Start comparison")) {
        try {
            this->benchmark.compare();
            this->errorMessage.clear();
        } catch(const std::exception& error) { this->errorMessage = error.what(); }
    }
    ImGui::EndDisabled();

    if(!this->errorMessage.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.48f, 0.43f, 1.0f));
        ImGui::TextWrapped("%s", this->errorMessage.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::TextWrapped("Each comparison runs all three searches once. Times measure only algorithm execution, with step recording disabled.");
    ImGui::TextWrapped("Files contain no coordinates. Distance heuristics use node IDs arranged in a fixed grid, scaled to the edge weights to preserve shortest paths.");
    if(this->benchmark.loaded() && this->benchmark.geometryScale() == 0.0)
        ImGui::TextWrapped("This graph requires zero distance estimates; Euclidean and Manhattan behave like Dijkstra.");
}

void BenchmarkView::drawResults() {
    ImGui::SeparatorText("Results");
    if(!this->benchmark.hasResults()) {
        ImGui::TextDisabled("Start the comparison to measure all three searches in milliseconds.");
        return;
    }

    ImGui::Text("Start: %zu | Goal: %zu", this->benchmark.start(), this->benchmark.goal());
    constexpr auto flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollX;
    const float tableWidth = 40.0f * ImGui::GetFontSize();
    const auto& style = ImGui::GetStyle();
    const float tableHeight = 4.0f * (ImGui::GetTextLineHeight() + 2.0f * style.CellPadding.y) + style.ScrollbarSize + 4.0f;
    if(ImGui::BeginTable("BenchmarkResults", 5, flags, ImVec2(0, tableHeight), tableWidth)) {
        ImGui::TableSetupColumn("Search");
        ImGui::TableSetupColumn("Time (ms)");
        ImGui::TableSetupColumn("Explored nodes");
        ImGui::TableSetupColumn("Path cost");
        ImGui::TableSetupColumn("Result");
        ImGui::TableHeadersRow();
        for(const auto& measurement : this->benchmark.results()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(heuristicName(measurement.heuristic));
            ImGui::TableNextColumn();
            ImGui::Text("%.6f", measurement.milliseconds);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", measurement.result.exploredNodes);
            ImGui::TableNextColumn();
            if(measurement.result.found) ImGui::Text("%.6g", measurement.result.totalCost);
            else ImGui::TextDisabled("--");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(measurement.result.found ? "Path found" : "Unreachable");
        }
        ImGui::EndTable();
    }
    ImGui::TextWrapped("Loading, graph preparation and rendering are excluded. Timings can vary between runs; press Start comparison to measure again.");
}

bool BenchmarkView::loadFile(const std::string& path) {
    try {
        if(path.empty()) throw std::invalid_argument("Enter a graph file path or choose Browse.");
        if(!std::filesystem::is_regular_file(path)) throw std::invalid_argument("Choose a regular graph file.");
        auto name = std::filesystem::path(path).filename().string();
        this->benchmark.load(GraphReader::read(path));
        this->loadedName = std::move(name);
        this->errorMessage.clear();
        return true;
    } catch(const std::exception& error) { this->errorMessage = error.what(); }
    return false;
}

void BenchmarkView::drawFilePicker() {
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(std::max(1.0f, std::min(640.0f, display.x - 32.0f)),
        std::max(1.0f, std::min(480.0f, display.y - 32.0f))), ImGuiCond_Appearing);
    if(!ImGui::BeginPopupModal("Choose benchmark graph", nullptr, ImGuiWindowFlags_NoSavedSettings)) return;
    ImGui::TextUnformatted("Directory");
    ImGui::SetNextItemWidth(-1);
    if(ImGui::InputText("##benchmarkDirectory", this->directoryPath.data(), this->directoryPath.size(),
        ImGuiInputTextFlags_EnterReturnsTrue)) this->refreshDirectory = true;
    if(ImGui::Button("Up one folder")) {
        const auto parent = std::filesystem::path(this->directoryPath.data()).parent_path();
        if(!parent.empty()) std::snprintf(this->directoryPath.data(), this->directoryPath.size(), "%s", parent.string().c_str());
        this->refreshDirectory = true;
    }
    ImGui::SameLine();
    if(ImGui::Button("Refresh")) this->refreshDirectory = true;
    ImGui::SameLine();
    if(ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
    if(this->refreshDirectory) {
        this->directoryEntries.clear();
        this->browseError.clear();
        try {
            for(const auto& entry : std::filesystem::directory_iterator(this->directoryPath.data())) {
                std::error_code error;
                const bool directory = entry.is_directory(error);
                if(!error && (directory || entry.is_regular_file(error)))
                    this->directoryEntries.emplace_back(entry.path().filename().string(), directory);
            }
            std::sort(this->directoryEntries.begin(), this->directoryEntries.end(), [](const auto& a, const auto& b) {
                return a.second == b.second ? a.first < b.first : a.second > b.second;
            });
        } catch(const std::exception& error) { this->browseError = error.what(); }
        this->refreshDirectory = false;
    }
    if(!this->browseError.empty()) ImGui::TextWrapped("%s", this->browseError.c_str());
    if(!this->errorMessage.empty()) ImGui::TextWrapped("%s", this->errorMessage.c_str());
    if(ImGui::BeginChild("BenchmarkFiles", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(this->directoryEntries.size()));
        while(clipper.Step()) {
            for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                const auto& [name, directory] = this->directoryEntries[static_cast<std::size_t>(i)];
                const auto label = (directory ? "[Folder] " : "") + name;
                ImGui::PushID(i);
                const bool selected = ImGui::Selectable(label.c_str());
                ImGui::PopID();
                if(!selected) continue;
                const auto path = (std::filesystem::path(this->directoryPath.data()) / name).string();
                if(directory) {
                    std::snprintf(this->directoryPath.data(), this->directoryPath.size(), "%s", path.c_str());
                    this->refreshDirectory = true;
                } else {
                    std::snprintf(this->filePath.data(), this->filePath.size(), "%s", path.c_str());
                    if(this->loadFile(path)) ImGui::CloseCurrentPopup();
                }
                break;
            }
        }
    }
    ImGui::EndChild();
    ImGui::EndPopup();
}
