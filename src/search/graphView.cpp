#include <graphView.hpp>

#include <graphReader.hpp>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr ImU32 background = IM_COL32(18, 23, 31, 255);
    constexpr ImU32 unseen = IM_COL32(61, 74, 94, 255);
    constexpr ImU32 visited = IM_COL32(48, 89, 143, 255);
    constexpr ImU32 frontier = IM_COL32(48, 183, 208, 255);
    constexpr ImU32 pathColor = IM_COL32(250, 198, 83, 255);
    constexpr ImU32 startColor = IM_COL32(71, 210, 143, 255);
    constexpr ImU32 goalColor = IM_COL32(244, 112, 130, 255);
    constexpr std::size_t maxFileBytes = 8 * 1024 * 1024;
    constexpr std::size_t maxVisibleEdges = 600;
    constexpr int matrixColumns = 16;

    const char* stateName(const Dijkstra::SearchStep& state, std::size_t node) {
        return state.visited[node] ? "Visited" : state.inFrontier[node] ? "Frontier" : "Unseen";
    }

    void legendItem(const char* label, ImU32 color) {
        const auto position = ImGui::GetCursorScreenPos();
        const auto size = ImGui::GetTextLineHeight();
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(position.x + size * 0.5f, position.y + size * 0.5f),
            size * 0.35f, color, 12);
        ImGui::Dummy(ImVec2(size, size));
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
    }

    void costText(double cost) {
        if(std::isfinite(cost)) ImGui::Text("%.6g", cost);
        else ImGui::TextDisabled("INF");
    }
}

void GraphView::draw() {
    const auto available = ImGui::GetContentRegionAvail();
    if(available.x <= 0.0f || available.y <= 0.0f) return;
    const auto spacing = ImGui::GetStyle().ItemSpacing;
    const bool stacked = available.x < 620.0f;
    const float sidebarWidth = std::min(300.0f, available.x * 0.43f);
    const float sidebarHeight = stacked ? std::max(1.0f, available.y * 0.46f) : available.y;
    if(ImGui::BeginChild("GraphSidebar", ImVec2(stacked ? available.x : sidebarWidth, sidebarHeight), ImGuiChildFlags_Borders))
        this->drawSidebar();
    ImGui::EndChild();

    // Process controls first: pressing Pause must stop this very frame.
    this->playback.update(ImGui::GetIO().DeltaTime);
    if(!stacked) ImGui::SameLine();
    const ImVec2 panelSize(stacked ? available.x : std::max(1.0f, available.x - sidebarWidth - spacing.x),
        stacked ? std::max(1.0f, available.y - sidebarHeight - spacing.y) : available.y);
    if(ImGui::BeginChild("GraphPanel", panelSize, ImGuiChildFlags_Borders,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        if(!this->playback.loaded()) {
            ImGui::SeparatorText("Graph visualization");
            ImGui::TextWrapped("Load an adjacency matrix or open the example to explore Dijkstra's shortest-path search.");
            ImGui::Spacing();
            ImGui::TextWrapped("Follow directed connections, inspect costs, and move nodes to arrange your graph.");
        } else {
            const char* status = this->playback.isPlaying() ? "Playing" : this->playback.finished()
                ? (this->playback.found() ? "Path found" : "No path") : this->playback.step() == 0 ? "Ready" : "Paused";
            ImGui::Text("%s | Explored: %zu", status, this->playback.step());
            if(this->playback.found())
                ImGui::Text("Cost: %.6g | %zu edges", this->playback.result().totalCost, this->playback.result().path.size() - 1);
            else ImGui::TextDisabled("%zu nodes | Dijkstra", this->positions.size());
            if(ImGui::BeginTabBar("GraphDetails")) {
                if(ImGui::BeginTabItem("Graph")) { this->drawCanvas(); ImGui::EndTabItem(); }
                if(ImGui::BeginTabItem("Node costs")) { this->drawCosts(); ImGui::EndTabItem(); }
                if(ImGui::BeginTabItem("Adjacency matrix")) { this->drawMatrix(); ImGui::EndTabItem(); }
                ImGui::EndTabBar();
            }
        }
    }
    ImGui::EndChild();
}

void GraphView::drawSidebar() {
    ImGui::SeparatorText("Load graph");
    ImGui::BeginDisabled(this->playback.isPlaying());
    ImGui::SetNextItemWidth(-1);
    const bool submitted = ImGui::InputTextWithHint("##graphPath", "Graph file path", this->filePath.data(), this->filePath.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);
    const float half = std::max(1.0f, (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f);
    if(ImGui::Button("Load graph", ImVec2(half, 0)) || submitted) this->loadFile(this->filePath.data());
    ImGui::SameLine();
    if(ImGui::Button("Browse...", ImVec2(half, 0))) {
        if(this->directoryPath[0] == '\0') {
            std::error_code error;
            auto directory = std::filesystem::current_path(error);
            if(error) directory = ".";
            std::snprintf(this->directoryPath.data(), this->directoryPath.size(), "%s", directory.string().c_str());
        }
        this->refreshDirectory = true;
        ImGui::OpenPopup("Choose graph file");
    }
    if(ImGui::Button("Load example", ImVec2(-1, 0))) this->loadExample();
    ImGui::EndDisabled();
    this->drawFilePicker();
    ImGui::TextDisabled("Up to %zu nodes / 8 MiB", GraphPlayback::maxNodes);
    if(!this->loadedName.empty()) ImGui::TextWrapped("Loaded: %s", this->loadedName.c_str());
    if(!this->loadError.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.52f, 0.52f, 1.0f));
        ImGui::TextWrapped("%s", this->loadError.c_str());
        ImGui::PopStyleColor();
    }
    if(ImGui::TreeNode("File format")) {
        ImGui::TextWrapped("Square matrix; each row is enclosed in |, with comma-separated weights. INF means no edge; 0 is a real weight. Negative weights are not supported.");
        ImGui::TextUnformatted("| 0, 4, INF |\n| INF, 0, 2 |\n| INF, INF, 0 |");
        ImGui::TreePop();
    }

    ImGui::SeparatorText("Playback");
    ImGui::BeginDisabled(!this->playback.loaded());
    ImGui::BeginDisabled(this->playback.isPlaying() || this->playback.finished());
    if(ImGui::Button("Play", ImVec2(half, 0))) this->playback.play();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!this->playback.isPlaying());
    if(ImGui::Button("Pause", ImVec2(half, 0))) this->playback.pause();
    ImGui::EndDisabled();
    ImGui::BeginDisabled(this->playback.step() == 0);
    if(ImGui::Button("Step back", ImVec2(half, 0))) this->playback.previous();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(this->playback.finished());
    if(ImGui::Button("Step forward", ImVec2(half, 0))) this->playback.next();
    ImGui::EndDisabled();
    if(ImGui::Button("Restart", ImVec2(-1, 0))) this->playback.restart();
    ImGui::SetItemTooltip("Return to step 0 and pause. Keeps node positions, endpoints, and speed.");
    ImGui::TextUnformatted("Speed (steps / second)");
    float speed = this->playback.speed();
    ImGui::SetNextItemWidth(-1);
    if(ImGui::SliderFloat("##graphSpeed", &speed, GraphPlayback::minSpeed, GraphPlayback::maxSpeed, "%.0f",
        ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) this->playback.setSpeed(speed);
    ImGui::TextDisabled("One step explores one node.");

    ImGui::SeparatorText("Search setup");
    ImGui::TextUnformatted("Dijkstra (no heuristic)");
    if(this->playback.isPlaying()) ImGui::TextWrapped("Pause to load a graph or change endpoints.");
    else ImGui::TextWrapped("Changing endpoints resets playback.");
    ImGui::BeginDisabled(this->playback.isPlaying());
    int start = static_cast<int>(this->playback.start()), goal = static_cast<int>(this->playback.goal());
    const int maximum = static_cast<int>(this->positions.size()) - 1;
    ImGui::TextUnformatted("Start node");
    ImGui::SetNextItemWidth(-1);
    if(ImGui::InputInt("##graphStart", &start)) {
        try {
            this->playback.setStart(static_cast<std::size_t>(std::clamp(start, 0, std::max(0, maximum))));
            this->loadError.clear();
        } catch(const std::exception& error) { this->loadError = error.what(); }
    }
    ImGui::TextUnformatted("Goal node");
    ImGui::SetNextItemWidth(-1);
    if(ImGui::InputInt("##graphGoal", &goal)) {
        try {
            this->playback.setGoal(static_cast<std::size_t>(std::clamp(goal, 0, std::max(0, maximum))));
            this->loadError.clear();
        } catch(const std::exception& error) { this->loadError = error.what(); }
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("View");
    ImGui::TextUnformatted("Node layout");
    const char* layouts[]{"Circular", "Random"};
    int selectedLayout = static_cast<int>(this->layout);
    ImGui::SetNextItemWidth(-1);
    if(ImGui::Combo("##graphLayout", &selectedLayout, layouts, 2)) {
        this->layout = static_cast<Layout>(selectedLayout);
        this->resetLayout();
    }
    ImGui::Checkbox("Node IDs", &this->showNodeIds);
    ImGui::Checkbox("Edge weights", &this->showWeights);
    ImGui::TextUnformatted("Zoom");
    float nextZoom = this->zoom;
    ImGui::SetNextItemWidth(-1);
    if(ImGui::SliderFloat("##graphZoom", &nextZoom, minZoom, maxZoom, "%.2fx",
        ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) this->setZoom(nextZoom);
    if(ImGui::Button("Fit graph", ImVec2(half, 0))) this->fitGraph();
    ImGui::SameLine();
    if(ImGui::Button(this->layout == Layout::Random ? "Randomize" : "Reset layout", ImVec2(half, 0))) this->resetLayout();
    ImGui::SetItemTooltip(this->layout == Layout::Random
        ? "Generate a new random layout. Keeps search progress and edge weights."
        : "Restore the circular layout. Keeps search progress and edge weights.");
    ImGui::EndDisabled();
    ImGui::TextWrapped("Left drag: move a node. Hover: node info. Wheel: zoom. Right or middle drag: pan.");
    ImGui::TextWrapped("Moving nodes only changes their layout, never edge weights.");

    ImGui::SeparatorText("Legend");
    if(ImGui::BeginTable("GraphLegend", 2)) {
        ImGui::TableNextColumn(); legendItem("Start", startColor);
        ImGui::TableNextColumn(); legendItem("Goal", goalColor);
        ImGui::TableNextColumn(); legendItem("Frontier", frontier);
        ImGui::TableNextColumn(); legendItem("Visited", visited);
        ImGui::TableNextColumn(); legendItem("Path", pathColor);
        ImGui::TableNextColumn(); legendItem("Unseen", unseen);
        ImGui::EndTable();
    }
    ImGui::TextWrapped("White ring: current node. Arrows show edge direction.");
    if(this->playback.loaded()) {
        ImGui::SeparatorText("Run details");
        ImGui::Text("Step %zu / %zu", this->playback.step(), this->playback.totalSteps());
        ImGui::Text("Search computation: %.3f ms", this->playback.elapsedMilliseconds());
        ImGui::SetItemTooltip("Time spent computing the complete Dijkstra run. Excludes animation and rendering.");
        if(this->playback.found()) {
            std::string route;
            for(const auto node : this->playback.result().path) {
                if(!route.empty()) route += " -> ";
                route += std::to_string(node);
            }
            ImGui::TextWrapped("Path: %s", route.c_str());
        } else if(this->playback.finished()) ImGui::TextWrapped("The goal is unreachable from the selected start.");
    }
}

void GraphView::drawFilePicker() {
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(std::max(1.0f, std::min(640.0f, display.x - 32.0f)),
        std::max(1.0f, std::min(480.0f, display.y - 32.0f))), ImGuiCond_Appearing);
    if(!ImGui::BeginPopupModal("Choose graph file", nullptr, ImGuiWindowFlags_NoSavedSettings)) return;
    ImGui::TextUnformatted("Directory");
    ImGui::SetNextItemWidth(-1);
    if(ImGui::InputText("##graphDirectory", this->directoryPath.data(), this->directoryPath.size(),
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
    if(ImGui::BeginChild("GraphFiles", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
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
                    this->loadFile(path);
                    ImGui::CloseCurrentPopup();
                }
                break;
            }
        }
    }
    ImGui::EndChild();
    ImGui::EndPopup();
}

void GraphView::loadFile(const std::string& path) {
    try {
        if(path.empty()) throw std::invalid_argument("Enter a graph file path or choose Browse.");
        if(!std::filesystem::is_regular_file(path)) throw std::runtime_error("Choose a regular graph file.");
        if(std::filesystem::file_size(path) > maxFileBytes) throw std::invalid_argument("Graph files must be 8 MiB or smaller.");
        // Bound the actual read as well as the metadata check, even if the file changes.
        std::ifstream file(path, std::ios::binary);
        if(!file) throw std::runtime_error("Could not open graph file: " + path);
        std::string contents(maxFileBytes + 1, '\0');
        file.read(contents.data(), static_cast<std::streamsize>(contents.size()));
        if(file.bad()) throw std::runtime_error("Could not read graph file: " + path);
        contents.resize(static_cast<std::size_t>(file.gcount()));
        if(contents.size() > maxFileBytes) throw std::invalid_argument("Graph files must be 8 MiB or smaller.");
        std::istringstream input(contents);
        this->playback.load(GraphReader::read(input));
        this->graphLoaded(std::filesystem::path(path).filename().string());
    } catch(const std::exception& error) { this->loadError = error.what(); }
}

void GraphView::loadExample() {
    constexpr auto inf = std::numeric_limits<double>::infinity();
    this->playback.load({
        {0, 4, 1, inf, inf, inf, inf, inf},
        {inf, 0, inf, 1, 7, inf, inf, inf},
        {inf, 2, 0, 5, inf, 8, inf, inf},
        {inf, inf, inf, 0, 3, 1, inf, inf},
        {inf, 2, inf, inf, 0, inf, 2, inf},
        {inf, inf, inf, inf, 0, 0, 6, 9},
        {inf, inf, inf, inf, inf, inf, 0, 1},
        {inf, inf, inf, inf, inf, inf, inf, 0}
    });
    this->graphLoaded("Example (8 nodes)");
}

void GraphView::graphLoaded(std::string name) {
    this->loadedName = std::move(name);
    this->loadError.clear();
    this->matrixPage = 0;
    this->edges.clear();
    const auto& graph = this->playback.graph();
    this->incoming.assign(graph.size(), 0);
    this->outgoing.assign(graph.size(), 0);
    for(std::size_t from = 0; from < graph.size(); ++from) {
        for(std::size_t to = 0; to < graph.size(); ++to) {
            if(!std::isfinite(graph[from][to])) continue;
            // Zero diagonal entries encode the trivial stay-put connection.
            // Keep them in the matrix, without obscuring every node with a loop.
            if(from == to && graph[from][to] == 0.0) continue;
            this->edges.push_back({from, to, graph[from][to]});
            ++this->outgoing[from];
            ++this->incoming[to];
        }
    }
    this->resetLayout();
}

void GraphView::resetLayout() {
    const auto count = this->playback.graph().size();
    this->positions.resize(count);
    if(this->layout == Layout::Random && count > 1) {
        static std::mt19937 generator(std::random_device{}());
        std::uniform_real_distribution<float> coordinate(-300.0f, 300.0f);
        for(std::size_t node = 0; node < count; ++node) {
            float bestDistance = -1.0f;
            // Pick the most separated of several random candidates to avoid
            // tight clusters while keeping each arrangement different.
            for(int attempt = 0; attempt < 24; ++attempt) {
                const Position candidate{coordinate(generator), coordinate(generator)};
                float nearest = std::numeric_limits<float>::infinity();
                for(std::size_t previous = 0; previous < node; ++previous) {
                    const auto dx = candidate.x - this->positions[previous].x;
                    const auto dy = candidate.y - this->positions[previous].y;
                    nearest = std::min(nearest, dx * dx + dy * dy);
                }
                if(nearest > bestDistance) {
                    bestDistance = nearest;
                    this->positions[node] = candidate;
                }
            }
        }
    } else {
        for(std::size_t node = 0; node < count; ++node) {
            const auto angle = 2.0 * std::numbers::pi * static_cast<double>(node) / static_cast<double>(count)
                - std::numbers::pi / 2.0;
            this->positions[node] = count == 1 ? Position{0, 0} : Position{
                300.0f * static_cast<float>(std::cos(angle)), 300.0f * static_cast<float>(std::sin(angle))};
        }
    }
    this->draggedNode.reset();
    this->fitGraph();
}

void GraphView::fitGraph() {
    if(this->positions.empty()) return;
    auto minimum = this->positions.front(), maximum = minimum;
    for(const auto& position : this->positions) {
        minimum.x = std::min(minimum.x, position.x); minimum.y = std::min(minimum.y, position.y);
        maximum.x = std::max(maximum.x, position.x); maximum.y = std::max(maximum.y, position.y);
    }
    this->viewCenter = {(minimum.x + maximum.x) * 0.5f, (minimum.y + maximum.y) * 0.5f};
    this->viewSpan = {std::max(100.0f, maximum.x - minimum.x), std::max(100.0f, maximum.y - minimum.y)};
    this->zoom = 1.0f;
    this->panX = this->panY = 0.0f;
}

void GraphView::setZoom(float value) {
    const auto next = std::clamp(value, minZoom, maxZoom);
    const auto ratio = next / this->zoom;
    this->panX *= ratio;
    this->panY *= ratio;
    this->zoom = next;
}

bool GraphView::onPath(std::size_t node) const {
    if(!this->playback.found()) return false;
    const auto& path = this->playback.result().path;
    return std::find(path.begin(), path.end(), node) != path.end();
}

void GraphView::drawCosts() {
    const auto& state = this->playback.state();
    const auto frontierCount = std::count(state.inFrontier.begin(), state.inFrontier.end(), true);
    ImGui::Text("Frontier: %zu | Visited: %zu", static_cast<std::size_t>(frontierCount), this->playback.step());
    ImGui::TextWrapped("g(n) is the cost from the start. Only visited costs are final. INF means undiscovered.");
    const auto available = ImGui::GetContentRegionAvail();
    if(available.y <= 1.0f) return;
    if(ImGui::BeginTable("GraphCosts", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY, available)) {
        ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Cost g(n)", ImGuiTableColumnFlags_WidthFixed, 110);
        ImGui::TableSetupColumn("Predecessor", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(this->positions.size()));
        while(clipper.Step()) {
            for(int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const auto node = static_cast<std::size_t>(row);
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%zu%s%s", node, node == this->playback.start() ? " S" : "", node == this->playback.goal() ? " G" : "");
                ImGui::TableNextColumn(); ImGui::TextUnformatted(stateName(state, node));
                ImGui::TableNextColumn(); costText(state.distances[node]);
                ImGui::TableNextColumn();
                if(node != this->playback.start() && std::isfinite(state.distances[node])) ImGui::Text("%zu", state.predecessors[node]);
                else ImGui::TextDisabled("--");
            }
        }
        ImGui::EndTable();
    }
}

void GraphView::drawMatrix() {
    const auto& graph = this->playback.graph();
    const auto count = static_cast<int>(graph.size());
    const auto pages = (count + matrixColumns - 1) / matrixColumns;
    ImGui::TextWrapped("Row = source, column = destination. INF = no edge; 0 is a valid weight.");
    if(pages > 1) {
        ImGui::SetNextItemWidth(-1);
        int page = this->matrixPage + 1;
        if(ImGui::SliderInt("##matrixPage", &page, 1, pages, "Column page %d", ImGuiSliderFlags_AlwaysClamp))
            this->matrixPage = page - 1;
    }
    const auto first = this->matrixPage * matrixColumns;
    const auto columns = std::min(matrixColumns, count - first);
    const auto available = ImGui::GetContentRegionAvail();
    if(available.y <= 1.0f) return;
    if(ImGui::BeginTable("GraphMatrix", columns + 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY, available)) {
        ImGui::TableSetupColumn("From / To", ImGuiTableColumnFlags_WidthFixed, 96);
        for(int column = 0; column < columns; ++column) {
            const auto label = std::to_string(first + column);
            ImGui::TableSetupColumn(label.c_str(), ImGuiTableColumnFlags_WidthFixed, 78);
        }
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableHeadersRow();
        ImGuiListClipper clipper;
        clipper.Begin(count);
        while(clipper.Step()) {
            for(int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%d", row);
                for(int column = 0; column < columns; ++column) {
                    if(ImGui::TableNextColumn()) costText(graph[static_cast<std::size_t>(row)][static_cast<std::size_t>(first + column)]);
                }
            }
        }
        ImGui::EndTable();
    }
}

void GraphView::drawCanvas() {
    const auto count = this->positions.size();
    if(this->edges.size() > maxVisibleEdges)
        ImGui::TextWrapped("Showing up to %zu of %zu edges; path and hovered node first. Inspect all weights in the matrix.", maxVisibleEdges, this->edges.size());
    else ImGui::TextDisabled("%zu directed edges | %.2fx zoom", this->edges.size(), this->zoom);
    if(this->showWeights && this->edges.size() > 80)
        ImGui::TextWrapped("Edge labels hidden for readability; use the matrix for weights.");

    const auto topLeft = ImGui::GetCursorScreenPos();
    const auto available = ImGui::GetContentRegionAvail();
    if(available.x < 1.0f || available.y < 1.0f) return;
    const ImVec2 bottomRight(topLeft.x + available.x, topLeft.y + available.y);
    const ImVec2 center(topLeft.x + available.x * 0.5f, topLeft.y + available.y * 0.5f);
    ImGui::InvisibleButton("GraphSurface", available,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
    const auto& io = ImGui::GetIO();
    if(hovered && io.MouseWheel != 0.0f) {
        const auto oldZoom = this->zoom;
        this->setZoom(this->zoom * std::pow(1.2f, io.MouseWheel));
        const auto ratio = this->zoom / oldZoom;
        this->panX += (io.MousePos.x - center.x) * (1.0f - ratio);
        this->panY += (io.MousePos.y - center.y) * (1.0f - ratio);
    }
    const bool draggingRight = ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0) && !ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    const bool draggingMiddle = ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0) && !ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
    if(active && (draggingRight || draggingMiddle)) {
        this->panX += io.MouseDelta.x;
        this->panY += io.MouseDelta.y;
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }
    const auto fit = std::max(0.001f, std::min(std::max(1.0f, available.x - 100.0f) / this->viewSpan.x,
        std::max(1.0f, available.y - 100.0f) / this->viewSpan.y));
    const float scale = fit * this->zoom;
    const auto screen = [&](std::size_t node) {
        const auto& position = this->positions[node];
        return ImVec2(center.x + this->panX + (position.x - this->viewCenter.x) * scale,
            center.y + this->panY + (position.y - this->viewCenter.y) * scale);
    };
    const float radius = count > 64 ? 12.0f : std::clamp(std::min(available.x, available.y) * 0.06f, 9.0f, 18.0f);
    std::optional<std::size_t> hoveredNode;
    if(hovered) {
        for(auto node = count; node-- > 0;) {
            const auto position = screen(node);
            const float dx = io.MousePos.x - position.x, dy = io.MousePos.y - position.y;
            if(dx * dx + dy * dy <= (radius + 4.0f) * (radius + 4.0f)) { hoveredNode = node; break; }
        }
    }
    if(active && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) this->draggedNode = hoveredNode;
    if(!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !active) this->draggedNode.reset();
    if(this->draggedNode && !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        auto& position = this->positions[*this->draggedNode];
        position.x += io.MouseDelta.x / scale;
        position.y += io.MouseDelta.y / scale;
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    } else if(hoveredNode) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    auto* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(topLeft, bottomRight, true);
    draw->AddRectFilled(topLeft, bottomRight, background);
    // rlImGui uses 16-bit indices without base-vertex support. Bounded edge
    // geometry and low-segment circles keep even a 512-node preview below 64K.
    const auto previousFlags = draw->Flags;
    if(count > 128 || this->edges.size() > 200)
        draw->Flags &= ~(ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill);
    const auto& state = this->playback.state();
    // Constant-time membership keeps dense completed graphs O(edges + nodes),
    // even when the shortest path traverses every node.
    std::vector<bool> pathNodes(count, false);
    if(this->playback.found())
        for(const auto node : this->playback.result().path) pathNodes[node] = true;
    const auto pathEdge = [&](const Edge& edge) {
        return edge.to != this->playback.start() && pathNodes[edge.to] && state.predecessors[edge.to] == edge.from;
    };
    const auto arrow = [&](ImVec2 tip, ImVec2 direction, ImU32 color) {
        const auto length = std::hypot(direction.x, direction.y);
        if(length < 0.01f) return;
        const ImVec2 unit(direction.x / length, direction.y / length);
        draw->AddTriangleFilled(tip, ImVec2(tip.x - unit.x * 9 - unit.y * 4, tip.y - unit.y * 9 + unit.x * 4),
            ImVec2(tip.x - unit.x * 9 + unit.y * 4, tip.y - unit.y * 9 - unit.x * 4), color);
    };
    const auto renderEdge = [&](const Edge& edge) {
        const bool isPathEdge = pathEdge(edge);
        const bool adjacent = hoveredNode && (*hoveredNode == edge.from || *hoveredNode == edge.to);
        const bool treeEdge = edge.to != this->playback.start() && std::isfinite(state.distances[edge.to]) && state.predecessors[edge.to] == edge.from;
        const auto color = isPathEdge ? pathColor : adjacent ? IM_COL32(191, 213, 238, 255)
            : treeEdge ? frontier : IM_COL32(91, 111, 139, 170);
        const auto a = screen(edge.from), b = screen(edge.to);
        ImVec2 label;
        if(edge.from == edge.to) {
            const ImVec2 loopCenter(a.x, a.y - radius - 10.0f);
            ImVec2 points[9];
            for(int i = 0; i <= 8; ++i) {
                const float angle = 0.35f + static_cast<float>(i) * 5.58f / 8.0f;
                points[i] = ImVec2(loopCenter.x + 14.0f * std::sin(angle), loopCenter.y + 14.0f * std::cos(angle));
            }
            draw->AddPolyline(points, 9, color, 0, 1.5f);
            arrow(points[8], ImVec2(points[8].x - points[7].x, points[8].y - points[7].y), color);
            label = ImVec2(loopCenter.x, loopCenter.y - 18.0f);
        } else {
            const float dx = b.x - a.x, dy = b.y - a.y, distance = std::hypot(dx, dy);
            if(distance <= radius * 2.0f + 2.0f) return;
            const ImVec2 unit(dx / distance, dy / distance);
            const bool reciprocal = std::isfinite(this->playback.graph()[edge.to][edge.from]);
            const float bend = reciprocal ? std::min(50.0f, distance * 0.2f) : 0.0f;
            const ImVec2 control((a.x + b.x) * 0.5f - unit.y * bend, (a.y + b.y) * 0.5f + unit.x * bend);
            const auto fromLength = std::hypot(control.x - a.x, control.y - a.y);
            const auto toLength = std::hypot(b.x - control.x, b.y - control.y);
            const ImVec2 begin(a.x + (control.x - a.x) / fromLength * (radius + 2), a.y + (control.y - a.y) / fromLength * (radius + 2));
            const ImVec2 end(b.x - (b.x - control.x) / toLength * (radius + 2), b.y - (b.y - control.y) / toLength * (radius + 2));
            draw->AddBezierQuadratic(begin, control, end, color, isPathEdge ? 3.0f : 1.5f, 8);
            arrow(end, ImVec2(end.x - control.x, end.y - control.y), color);
            label = ImVec2(0.25f * begin.x + 0.5f * control.x + 0.25f * end.x,
                0.25f * begin.y + 0.5f * control.y + 0.25f * end.y);
        }
        if(this->showWeights && this->edges.size() <= 80) {
            char text[32]; std::snprintf(text, sizeof(text), "%.4g", edge.weight);
            const auto size = ImGui::CalcTextSize(text);
            const ImVec2 textPos(label.x - size.x * 0.5f, label.y - size.y * 0.5f);
            draw->AddRectFilled(ImVec2(textPos.x - 3, textPos.y - 1), ImVec2(textPos.x + size.x + 3, textPos.y + size.y + 1), background);
            draw->AddText(textPos, isPathEdge ? pathColor : IM_COL32(209, 220, 235, 255), text);
        }
    };
    // Prioritize the completed path and hovered connections when a graph is
    // too dense to draw in full. The matrix always exposes every input weight.
    std::vector<const Edge*> visible;
    visible.reserve(std::min(maxVisibleEdges, this->edges.size()));
    for(const auto& edge : this->edges) if(pathEdge(edge)) visible.push_back(&edge);
    if(hoveredNode) {
        for(const auto& edge : this->edges) {
            if(visible.size() == maxVisibleEdges) break;
            if(!pathEdge(edge) && (*hoveredNode == edge.from || *hoveredNode == edge.to)) visible.push_back(&edge);
        }
    }
    for(const auto& edge : this->edges) {
        if(visible.size() == maxVisibleEdges) break;
        if(!pathEdge(edge) && (!hoveredNode || (*hoveredNode != edge.from && *hoveredNode != edge.to))) visible.push_back(&edge);
    }
    for(auto edge = visible.rbegin(); edge != visible.rend(); ++edge) renderEdge(**edge);

    for(std::size_t node = 0; node < count; ++node) {
        const auto position = screen(node);
        if(position.x + radius + 6 < topLeft.x || position.x - radius - 6 > bottomRight.x ||
            position.y + radius + 6 < topLeft.y || position.y - radius - 6 > bottomRight.y) continue;
        const bool isStart = node == this->playback.start(), isGoal = node == this->playback.goal();
        const auto color = isStart ? startColor : isGoal ? goalColor : pathNodes[node] ? pathColor :
            state.visited[node] ? visited : state.inFrontier[node] ? frontier : unseen;
        draw->AddCircleFilled(position, radius, color, 12);
        draw->AddCircle(position, radius, IM_COL32(197, 212, 230, 220), 12, 1.0f);
        if(this->playback.step() > 0 && state.current == node) draw->AddCircle(position, radius + 4, IM_COL32_WHITE, 12, 2);
        if(isStart && isGoal) draw->AddCircle(position, radius + 2, goalColor, 12, 2);
        if(this->showNodeIds) {
            char text[32]; std::snprintf(text, sizeof(text), "%zu", node);
            const auto size = ImGui::CalcTextSize(text);
            const bool lightFill = isStart || isGoal || pathNodes[node] || state.inFrontier[node];
            draw->AddText(ImVec2(position.x - size.x * 0.5f, position.y - size.y * 0.5f), lightFill ? background : IM_COL32_WHITE, text);
        }
        if((isStart || isGoal) && radius >= 14.0f) {
            const char* text = isStart && isGoal ? "START / GOAL" : isStart ? "START" : "GOAL";
            const auto size = ImGui::CalcTextSize(text);
            draw->AddText(ImVec2(position.x - size.x * 0.5f, position.y + radius + 6), color, text);
        }
    }
    draw->Flags = previousFlags;
    draw->PopClipRect();
    if(hoveredNode && !this->draggedNode) {
        const auto node = *hoveredNode;
        ImGui::BeginTooltip();
        ImGui::Text("Node %zu%s%s", node, node == this->playback.start() ? " | Start" : "", node == this->playback.goal() ? " | Goal" : "");
        ImGui::Text("State: %s%s", stateName(state, node), this->onPath(node) ? " | Shortest path" : "");
        if(std::isfinite(state.distances[node])) ImGui::Text("Cost g(n): %.8g%s", state.distances[node], state.visited[node] ? " (final)" : " (tentative)");
        else ImGui::TextUnformatted("Cost g(n): INF (undiscovered)");
        if(node != this->playback.start() && std::isfinite(state.distances[node])) ImGui::Text("Predecessor: %zu", state.predecessors[node]);
        else ImGui::TextUnformatted("Predecessor: --");
        ImGui::Text("Edges in: %zu | out: %zu", this->incoming[node], this->outgoing[node]);
        ImGui::TextDisabled("Left drag to move");
        ImGui::EndTooltip();
    }
}
