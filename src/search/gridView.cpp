#include <gridView.hpp>

#include <imgui.h>
#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {
    constexpr Color openColor{32, 40, 52, 255};
    constexpr Color wallColor{116, 129, 149, 255};
    constexpr Color visitedColor{48, 89, 143, 255};
    constexpr Color frontierColor{48, 183, 208, 255};
    constexpr Color pathColor{250, 198, 83, 255};
    constexpr Color startColor{71, 210, 143, 255};
    constexpr Color goalColor{244, 112, 130, 255};
    constexpr Color backgroundColor{18, 23, 31, 255};

    ImU32 packed(Color color) { return IM_COL32(color.r, color.g, color.b, color.a); }

    void legendItem(const char* label, Color color) {
        const auto position = ImGui::GetCursorScreenPos();
        const auto size = ImGui::GetTextLineHeight();
        ImGui::GetWindowDrawList()->AddRectFilled(position, ImVec2(position.x + size, position.y + size), packed(color), 3.0f);
        ImGui::Dummy(ImVec2(size, size));
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
    }
}

struct GridView::TextureData {
    Texture2D image{};
    std::vector<Color> pixels;
    std::size_t revision = std::numeric_limits<std::size_t>::max();
    ~TextureData() { if(this->image.id != 0) UnloadTexture(this->image); }
};

GridView::GridView() : texture(std::make_unique<TextureData>()) {}
GridView::~GridView() = default;

void GridView::draw() {
    const auto available = ImGui::GetContentRegionAvail();
    if(available.x <= 0.0f || available.y <= 0.0f) return;
    const auto spacing = ImGui::GetStyle().ItemSpacing;
    const float sidebarWidth = std::min(270.0f, available.x * 0.42f);
    const bool stacked = available.x < 540.0f;
    const float sidebarHeight = stacked ? std::max(1.0f, available.y * 0.45f) : available.y;
    if(ImGui::BeginChild("GridSidebar", ImVec2(stacked ? available.x : sidebarWidth, sidebarHeight), ImGuiChildFlags_Borders))
        this->drawSidebar();
    ImGui::EndChild();

    // Apply playback after controls so Pause prevents this frame's advancement.
    this->playback.update(ImGui::GetIO().DeltaTime);
    if(!stacked) ImGui::SameLine();
    const ImVec2 canvasSize(stacked ? available.x : std::max(1.0f, available.x - sidebarWidth - spacing.x),
        stacked ? std::max(1.0f, available.y - sidebarHeight - spacing.y) : available.y);
    if(ImGui::BeginChild("GridCanvasPanel", canvasSize, ImGuiChildFlags_Borders,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        this->drawCanvas();
    ImGui::EndChild();
}

void GridView::drawSidebar() {
    ImGui::SeparatorText("Playback");
    const float half = std::max(1.0f, (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f);
    ImGui::BeginDisabled(this->playback.isPlaying() || this->playback.finished());
    if(ImGui::Button("Play", ImVec2(half, 0))) this->playback.play();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!this->playback.isPlaying());
    if(ImGui::Button("Pause", ImVec2(half, 0))) this->playback.pause();
    ImGui::EndDisabled();

    ImGui::BeginDisabled(this->playback.step() == 0);
    if(ImGui::Button("Previous step", ImVec2(half, 0))) this->playback.previous();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(this->playback.finished());
    if(ImGui::Button("Next step", ImVec2(half, 0))) this->playback.next();
    ImGui::EndDisabled();
    if(ImGui::Button("Restart", ImVec2(-1, 0))) this->playback.restart();
    ImGui::SetItemTooltip("Return to step 0 and pause. Keeps obstacles, endpoints, and heuristic.");
    ImGui::TextUnformatted("Speed (steps / second)");
    float speed = this->playback.speed();
    ImGui::SetNextItemWidth(-1);
    if(ImGui::SliderFloat("##speed", &speed, GridPlayback::minSpeed, GridPlayback::maxSpeed, "%.0f",
        ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) this->playback.setSpeed(speed);

    ImGui::SeparatorText("Search setup");
    ImGui::TextWrapped(this->playback.isPlaying() ? "Editing locked while playing." : "Edits reset the search to step 0.");
    ImGui::BeginDisabled(this->playback.isPlaying());
    ImGui::TextUnformatted("Heuristic");
    if(ImGui::RadioButton("None (Dijkstra)", this->playback.heuristic() == Heuristics::Type::None))
        this->playback.setHeuristic(Heuristics::Type::None);
    if(ImGui::RadioButton("Manhattan (A*)", this->playback.heuristic() == Heuristics::Type::Manhattan))
        this->playback.setHeuristic(Heuristics::Type::Manhattan);
    if(ImGui::RadioButton("Euclidean (A*)", this->playback.heuristic() == Heuristics::Type::Euclidean))
        this->playback.setHeuristic(Heuristics::Type::Euclidean);

    ImGui::Spacing();
    ImGui::TextUnformatted("Grid size (total cells)");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderInt("##cells", &this->requestedCells, GridPlayback::minCells, GridPlayback::maxCells, "%d",
        ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp);
    // Commit on release (including typed values), rather than reallocating during a drag.
    if(ImGui::IsItemDeactivatedAfterEdit()) {
        this->requestedCells = std::clamp(this->requestedCells, GridPlayback::minCells, GridPlayback::maxCells);
        if(this->playback.resize(static_cast<std::size_t>(this->requestedCells))) {
            this->zoom = 1.0f;
            this->panX = this->panY = 0.0f;
            this->lastPainted.reset();
        }
    }
    ImGui::SetItemTooltip("30 to 10,000,000 total cells. Ctrl+click to type an exact size. Resizing clears the map.");
    ImGui::TextDisabled("%zu rows x %zu columns", this->playback.grid().rows(), this->playback.grid().columns());
    if(this->playback.grid().size() % this->playback.grid().columns() != 0)
        ImGui::TextDisabled("Last row is partially filled.");

    const char* tools[]{"Obstacle", "Erase", "Start", "Goal"};
    ImGui::TextUnformatted("Paint tool");
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##paintTool", &this->editTool, tools, 4);
    if(ImGui::Button("Clear obstacles", ImVec2(-1, 0))) this->playback.clearObstacles();
    ImGui::EndDisabled();

    ImGui::SeparatorText("View");
    ImGui::TextUnformatted("Zoom");
    float nextZoom = this->zoom;
    ImGui::SetNextItemWidth(-1);
    if(ImGui::SliderFloat("##zoom", &nextZoom, minZoom, maxZoom, "%.2fx",
        ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) this->setZoom(nextZoom);
    if(ImGui::Button("Fit grid", ImVec2(-1, 0))) {
        this->zoom = 1.0f;
        this->panX = this->panY = 0.0f;
    }
    ImGui::TextWrapped("Wheel: zoom at pointer. Right or middle drag: pan. Left drag: paint.");

    ImGui::SeparatorText("Legend");
    if(ImGui::BeginTable("GridLegend", 2)) {
        ImGui::TableNextColumn(); legendItem("Start", startColor);
        ImGui::TableNextColumn(); legendItem("Goal", goalColor);
        ImGui::TableNextColumn(); legendItem("Frontier", frontierColor);
        ImGui::TableNextColumn(); legendItem("Visited", visitedColor);
        ImGui::TableNextColumn(); legendItem("Path", pathColor);
        ImGui::TableNextColumn(); legendItem("Obstacle", wallColor);
        ImGui::EndTable();
    }
}

void GridView::setZoom(float value) {
    const auto next = std::clamp(value, minZoom, maxZoom);
    const auto ratio = next / this->zoom;
    this->panX *= ratio;
    this->panY *= ratio;
    this->zoom = next;
}

void GridView::refreshTexture() {
    if(this->texture->revision == this->playback.revision()) return;
    const auto& grid = this->playback.grid();
    const auto width = static_cast<int>(grid.columns());
    const auto height = static_cast<int>(grid.rows());
    auto& pixels = this->texture->pixels;
    pixels.assign(static_cast<std::size_t>(width * height), backgroundColor);
    for(std::size_t node = 0; node < grid.size(); ++node) {
        Color color = openColor;
        if(grid.isBlocked(node)) color = wallColor;
        else if(this->playback.onPath(node)) color = pathColor;
        else if(this->playback.state(node) == GridPlayback::CellState::Visited) color = visitedColor;
        else if(this->playback.state(node) == GridPlayback::CellState::Frontier) color = frontierColor;
        if(node == this->playback.start()) color = startColor;
        if(node == this->playback.goal()) color = goalColor;
        pixels[node] = color;
    }
    auto& texture = this->texture->image;
    if(texture.width != width || texture.height != height || texture.id == 0) {
        if(texture.id != 0) UnloadTexture(texture);
        // One pixel per cell keeps rendering independent of the grid's cell count
        // and avoids Dear ImGui's 16-bit draw-index limit.
        const Image image{pixels.data(), width, height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        texture = LoadTextureFromImage(image);
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    } else UpdateTexture(texture, pixels.data());
    this->texture->revision = this->playback.revision();
}

void GridView::paint(std::size_t node) {
    if(this->editTool == 2) { this->playback.setStart(node); return; }
    if(this->editTool == 3) { this->playback.setGoal(node); return; }
    const auto columns = this->playback.grid().columns();
    auto from = this->lastPainted.value_or(node);
    int x = static_cast<int>(from % columns), y = static_cast<int>(from / columns);
    const int endX = static_cast<int>(node % columns), endY = static_cast<int>(node / columns);
    const int dx = std::abs(endX - x), dy = -std::abs(endY - y);
    const int sx = x < endX ? 1 : -1, sy = y < endY ? 1 : -1;
    int error = dx + dy;
    // Fill gaps between mouse samples so fast drags paint continuous strokes.
    for(;;) {
        const auto cell = static_cast<std::size_t>(y) * columns + static_cast<std::size_t>(x);
        if(cell < this->playback.grid().size()) this->playback.setBlocked(cell, this->editTool == 0);
        if(x == endX && y == endY) break;
        const int twice = 2 * error;
        if(twice >= dy) { error += dy; x += sx; }
        if(twice <= dx) { error += dx; y += sy; }
    }
    this->lastPainted = node;
}

void GridView::drawCanvas() {
    const char* status = this->playback.isPlaying() ? "Playing" : this->playback.finished()
        ? (this->playback.found() ? "Path found" : "No path") : this->playback.step() == 0 ? "Ready" : "Paused";
    ImGui::Text("%s | Step %zu", status, this->playback.step());
    if(this->playback.found()) ImGui::Text("Path length: %zu moves", this->playback.path().size() - 1);
    else ImGui::TextDisabled("%zu cells | %.2fx zoom", this->playback.grid().size(), this->zoom);

    const auto topLeft = ImGui::GetCursorScreenPos();
    const auto available = ImGui::GetContentRegionAvail();
    if(available.x < 1.0f || available.y < 1.0f) return;
    const ImVec2 bottomRight(topLeft.x + available.x, topLeft.y + available.y);
    const ImVec2 center(topLeft.x + available.x * 0.5f, topLeft.y + available.y * 0.5f);
    ImGui::InvisibleButton("GridSurface", available,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const auto& io = ImGui::GetIO();
    if(hovered && io.MouseWheel != 0.0f) {
        const auto oldZoom = this->zoom;
        this->setZoom(this->zoom * std::pow(1.2f, io.MouseWheel));
        const auto ratio = this->zoom / oldZoom;
        this->panX += (io.MousePos.x - center.x) * (1.0f - ratio);
        this->panY += (io.MousePos.y - center.y) * (1.0f - ratio);
    }
    // The press frame's delta may include movement from outside the canvas.
    const bool draggingRight = ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0) && !ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    const bool draggingMiddle = ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0) && !ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
    if(active && (draggingRight || draggingMiddle)) {
        this->panX += io.MouseDelta.x;
        this->panY += io.MouseDelta.y;
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    const auto& grid = this->playback.grid();
    const float fit = std::max(0.001f, std::min(std::max(1.0f, available.x - 24.0f) / grid.columns(),
        std::max(1.0f, available.y - 24.0f) / grid.rows()));
    const float cellSize = fit * this->zoom;
    const float width = cellSize * grid.columns(), height = cellSize * grid.rows();
    const float panLimitX = std::max(0.0f, (width + available.x) * 0.5f - 24.0f);
    const float panLimitY = std::max(0.0f, (height + available.y) * 0.5f - 24.0f);
    this->panX = std::clamp(this->panX, -panLimitX, panLimitX);
    this->panY = std::clamp(this->panY, -panLimitY, panLimitY);
    const ImVec2 origin(center.x + this->panX - width * 0.5f, center.y + this->panY - height * 0.5f);
    std::optional<std::size_t> hoveredCell;
    if(hovered && io.MousePos.x >= origin.x && io.MousePos.y >= origin.y) {
        const auto column = static_cast<std::size_t>((io.MousePos.x - origin.x) / cellSize);
        const auto row = static_cast<std::size_t>((io.MousePos.y - origin.y) / cellSize);
        const auto node = row * grid.columns() + column;
        if(column < grid.columns() && row < grid.rows() && node < grid.size()) hoveredCell = node;
    }
    if(!this->playback.isPlaying() && hoveredCell && active && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if(this->editTool < 2 || ImGui::IsMouseClicked(ImGuiMouseButton_Left)) this->paint(*hoveredCell);
    } else this->lastPainted.reset();

    this->refreshTexture();
    auto* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(topLeft, bottomRight, true);
    draw->AddRectFilled(topLeft, bottomRight, packed(backgroundColor));
    draw->AddImage(static_cast<ImTextureID>(this->texture->image.id), origin, ImVec2(origin.x + width, origin.y + height));

    // Draw only visible grid lines, and suppress them when cells are too small.
    if(cellSize >= 8.0f) {
        const auto firstColumn = static_cast<int>(std::clamp(std::floor((topLeft.x - origin.x) / cellSize), 0.0f, static_cast<float>(grid.columns())));
        const auto lastColumn = static_cast<int>(std::clamp(std::ceil((bottomRight.x - origin.x) / cellSize), 0.0f, static_cast<float>(grid.columns())));
        const auto firstRow = static_cast<int>(std::clamp(std::floor((topLeft.y - origin.y) / cellSize), 0.0f, static_cast<float>(grid.rows())));
        const auto lastRow = static_cast<int>(std::clamp(std::ceil((bottomRight.y - origin.y) / cellSize), 0.0f, static_cast<float>(grid.rows())));
        const auto line = IM_COL32(12, 17, 24, 155);
        for(int column = firstColumn; column <= lastColumn; ++column) {
            const auto x = origin.x + column * cellSize;
            draw->AddLine(ImVec2(x, std::max(topLeft.y, origin.y)), ImVec2(x, std::min(bottomRight.y, origin.y + height)), line);
        }
        for(int row = firstRow; row <= lastRow; ++row) {
            const auto y = origin.y + row * cellSize;
            draw->AddLine(ImVec2(std::max(topLeft.x, origin.x), y), ImVec2(std::min(bottomRight.x, origin.x + width), y), line);
        }
    }
    const auto mark = [&](std::size_t node, const char* label, Color color) {
        const ImVec2 cell(origin.x + (node % grid.columns()) * cellSize, origin.y + (node / grid.columns()) * cellSize);
        if(cellSize >= 18.0f) {
            const auto text = ImGui::CalcTextSize(label);
            draw->AddText(ImVec2(cell.x + (cellSize - text.x) * 0.5f, cell.y + (cellSize - text.y) * 0.5f), packed(backgroundColor), label);
        } else if(cellSize < 5.0f) {
            draw->AddCircle(ImVec2(cell.x + cellSize * 0.5f, cell.y + cellSize * 0.5f), 4.0f, packed(color), 12, 2.0f);
        }
    };
    mark(this->playback.start(), "S", startColor);
    mark(this->playback.goal(), "G", goalColor);
    if(this->playback.step() > 0 && !this->playback.finished()) {
        const auto node = this->playback.current();
        const ImVec2 cell(origin.x + (node % grid.columns()) * cellSize, origin.y + (node / grid.columns()) * cellSize);
        draw->AddRect(cell, ImVec2(cell.x + cellSize, cell.y + cellSize), IM_COL32_WHITE, 0, 0, 2.0f);
    }
    if(hoveredCell && !this->playback.isPlaying()) {
        const auto node = *hoveredCell;
        const ImVec2 cell(origin.x + (node % grid.columns()) * cellSize, origin.y + (node / grid.columns()) * cellSize);
        draw->AddRect(cell, ImVec2(cell.x + cellSize, cell.y + cellSize), IM_COL32_WHITE);
        ImGui::SetTooltip("Row %zu, column %zu%s", node / grid.columns() + 1, node % grid.columns() + 1,
            grid.isBlocked(node) ? " | Obstacle" : "");
    }
    draw->PopClipRect();
}
