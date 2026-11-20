#include <coloringView.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <imgui.h>

namespace coloring {
std::vector<ColoringView::PaletteColor> ColoringView::makePalette(int count) {
    static const PaletteColor presets[]{
        {"Coral", IM_COL32(242, 135, 120, 255)}, {"Teal", IM_COL32(77, 191, 177, 255)},
        {"Gold", IM_COL32(244, 196, 90, 255)}, {"Red", IM_COL32(227, 77, 72, 255)},
        {"Blue", IM_COL32(93, 158, 238, 255)}, {"Green", IM_COL32(108, 182, 98, 255)},
        {"Yellow", IM_COL32(246, 218, 94, 255)}, {"Purple", IM_COL32(175, 138, 220, 255)},
        {"Orange", IM_COL32(246, 163, 85, 255)}, {"Pink", IM_COL32(237, 147, 189, 255)},
        {"White", IM_COL32(245, 245, 245, 255)}, {"Black", IM_COL32(24, 25, 29, 255)}
    };
    std::vector<PaletteColor> colors;
    colors.reserve(count);
    for (int i = 0; i < count; ++i) {
        if (i < static_cast<int>(std::size(presets))) {
            colors.push_back(presets[i]);
        } else {
            const float hue = std::fmod(0.59f + i * 0.618034f, 1.0f);
            colors.push_back({"Color" + std::to_string(i + 1), ImColor::HSV(hue, 0.52f, 0.89f)});
        }
    }
    return colors;
}

void ColoringView::drawDiagram() {
    const Run* result = selected < 0 ? nullptr : &*runs[selected];
    ImGui::SeparatorText("Graph");
    if (result) {
        ImGui::TextWrapped("%s / %s", algorithmNames[selected / 4], modeNames[selected % 4]);
        if (result->success()) ImGui::TextWrapped("Found a %d-coloring using %d colors.", k, result->used);
        else {
            ImGui::TextWrapped("This strategy found no %d-coloring. Blocked at %s.", k, graph.labels[result->failedVertex].c_str());
            ImGui::TextWrapped("Colored neighbors: %s", blockersText(*result).c_str());
        }
    }
    const auto count = graph.labels.size();
    const float halfHeight = ImGui::GetTextLineHeight() * 0.85f;
    float halfWidth = halfHeight;
    for (const auto& label : graph.labels)
        halfWidth = std::max(halfWidth, ImGui::CalcTextSize(label.c_str()).x / 2 + 8);
    const float canvasHeight = std::clamp(ImGui::GetContentRegionAvail().y * 0.68f, 150.0f, 440.0f);
    if (ImGui::BeginChild("Diagram", ImVec2(0, canvasHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
        const ImVec2 available = ImGui::GetContentRegionAvail();
        // Keep neighboring labels apart. Large diagrams scroll inside this panel.
        const float minimumRadius = count <= 1 ? 0 : (halfWidth + halfHeight + 4) / std::sin(std::numbers::pi / count);
        const float pad = 7;
        const float rx = std::max(minimumRadius, available.x / 2 - halfWidth - pad);
        const float ry = std::max(minimumRadius, available.y / 2 - halfHeight - pad);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const ImVec2 center(origin.x + rx + halfWidth + pad, origin.y + ry + halfHeight + pad);
        std::vector<ImVec2> points;
        for (std::size_t v = 0; v < count; ++v) {
            const double angle = 2.0 * std::numbers::pi * v / count - std::numbers::pi / 2;
            points.emplace_back(center.x + (count > 1 ? rx * std::cos(angle) : 0),
                                center.y + (count > 1 ? ry * std::sin(angle) : 0));
        }
        auto* draw = ImGui::GetWindowDrawList();
        for (auto [a, b] : graph.edges)
            draw->AddLine(points[a], points[b], ImGui::GetColorU32(ImGuiCol_TextDisabled), 2.0f);
        for (std::size_t v = 0; v < count; ++v) {
            const auto p = points[v];
            const auto size = ImGui::CalcTextSize(graph.labels[v].c_str());
            const float w = std::max(halfHeight, size.x / 2 + 8);
            const ImVec2 lo(p.x - w, p.y - halfHeight), hi(p.x + w, p.y + halfHeight);
            const int c = result ? result->colors[v] : -1;
            const ImU32 fill = c < 0 ? ImGui::GetColorU32(ImGuiCol_FrameBg) : palette[c].swatch;
            draw->AddRectFilled(lo, hi, fill, halfHeight);
            const bool failed = result && static_cast<int>(v) == result->failedVertex;
            draw->AddRect(lo, hi, failed ? IM_COL32(255, 133, 111, 255) : ImGui::GetColorU32(ImGuiCol_TextDisabled),
                          halfHeight, failed ? 3.0f : 1.0f);
            const ImVec4 rgb = ImGui::ColorConvertU32ToFloat4(fill);
            const auto ink = (rgb.x * 0.299f + rgb.y * 0.587f + rgb.z * 0.114f > 0.52f)
                ? IM_COL32(20, 27, 35, 255) : IM_COL32_WHITE;
            draw->AddText(ImVec2(p.x - size.x / 2, p.y - size.y / 2), ink, graph.labels[v].c_str());
            if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(lo, hi))
                ImGui::SetTooltip("%s: %s", graph.labels[v].c_str(), c < 0 ? "Uncolored" : palette[c].name.c_str());
        }
        ImGui::Dummy(ImVec2(2 * (rx + halfWidth + pad), 2 * (ry + halfHeight + pad)));
    }
    ImGui::EndChild();
    if (result) ImGui::TextWrapped("Assignment order: %s", orderText(*result).c_str());
    const float panelRight = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    for (std::size_t c = 0; c < palette.size(); ++c) {
        const float width = ImGui::GetFrameHeight() + ImGui::CalcTextSize(palette[c].name.c_str()).x + 32;
        if (c && ImGui::GetItemRectMax().x + width < panelRight)
            ImGui::SameLine();
        ImGui::PushID(static_cast<int>(c));
        ImGui::ColorButton("##swatch", ImGui::ColorConvertU32ToFloat4(palette[c].swatch),
                           ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(15, 15));
        ImGui::SameLine(0, 5);
        ImGui::TextUnformatted(palette[c].name.c_str());
        ImGui::PopID();
    }
}
} // namespace coloring
