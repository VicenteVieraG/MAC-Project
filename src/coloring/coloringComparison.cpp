#include <coloringView.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace coloring {
namespace {
std::string field(const std::string& value) {
    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"') escaped += '"';
        escaped += ch;
    }
    return escaped + '"';
}

std::string join(const std::vector<std::string>& items) {
    std::string text;
    for (const auto& item : items) {
        if (!text.empty()) text += ' ';
        text += item;
    }
    return text;
}
}

std::string ColoringView::paletteText() const {
    std::string names;
    for (const auto& color : palette) {
        if (!names.empty()) names += ' ';
        names += color.name;
    }
    return names;
}

std::string ColoringView::csv() const {
    std::ostringstream out;
    out << "graph,vertices,edges,vertex_order,edge_list,k,palette,polynomial,chi,p_at_k,exact_calls,exact_ms,"
           "algorithm,mode,success,colored,colors_used,partial,checks,median_ms,mrv_selections,mrv_ties,"
           "lcv_evaluations,lcv_ties,lcv_changes,assignment_order,assignments,failed_vertex,colored_neighbors\n";
    std::string edges;
    for (auto [a, b] : graph.edges) edges += graph.labels[a] + " " + graph.labels[b] + "\n";
    for (const auto& entry : runs) {
        if (!entry) continue;
        const auto& r = *entry;
        out << field(graphName) << ',' << graph.labels.size() << ',' << graph.edges.size() << ','
            << field(join(graph.labels)) << ',' << field(edges) << ',' << k << ',' << field(paletteText()) << ',';
        if (exact) out << field(exact->formula()) << ',' << exact->chromaticNumber << ',' << exact->evaluate(k)
                       << ',' << exact->calls << ',' << exact->milliseconds;
        else out << ",,,,";
        std::vector<std::string> assignments;
        for (int v : r.order) assignments.push_back(graph.labels[v] + "=" + palette[r.colors[v]].name);
        out << ',' << algorithmNames[static_cast<int>(r.algorithm)] << ',' << modeNames[static_cast<int>(r.mode)]
            << ',' << r.success() << ',' << r.order.size() << ',' << r.used << ',' << !r.success() << ',' << r.checks
            << ',' << std::setprecision(9) << r.milliseconds << ',' << r.mrvSelections << ',' << r.mrvTies
            << ',' << r.lcvEvaluations << ',' << r.lcvTies << ',' << r.lcvChanges << ',' << field(orderText(r))
            << ',' << field(join(assignments)) << ',' << field(r.success() ? "" : graph.labels[r.failedVertex])
            << ',' << field(blockersText(r)) << '\n';
    }
    return out.str();
}

void ColoringView::drawComparison() {
    ImGui::TextWrapped("k = %d   /   %s", k, paletteText().c_str());
    if (ImGui::Button("Compare all 12")) {
        if (!exact && graph.labels.size() <= maxExactVertices) calculateExact();
        for (int a = 0; a < 3; ++a)
            for (int m = 0; m < 4; ++m) run(a, m);
        selected = algorithm * 4 + mode;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(selected < 0);
    if (ImGui::Button("Clear runs")) clearRuns();
    ImGui::EndDisabled();
    if (exact) ImGui::TextWrapped("Chromatic number = %d    P(G,%d) = %lld", exact->chromaticNumber, k,
                                  static_cast<long long>(exact->evaluate(k)));
    else if (!exactError.empty()) ImGui::TextWrapped("%s", exactError.c_str());
    else if (graph.labels.size() > maxExactVertices) ImGui::TextWrapped("Exact reference is limited to 10 vertices.");
    if (selected < 0) return;
    ImGui::TextWrapped("Select a run to inspect its graph in Coloring. Times are medians of five runs.");
    constexpr const char* headings[]{"Algorithm", "Mode", "Result", "Colored", "Colors", "Checks", "ms",
                                     "MRV picks", "MRV ties", "LCV scores", "LCV ties", "LCV changes"};
    const float height = std::clamp(ImGui::GetContentRegionAvail().y * 0.62f, 130.0f, 440.0f);
    if (ImGui::BeginTable("RunMetrics", 12, ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV,
                          ImVec2(0, height))) {
        for (auto heading : headings) ImGui::TableSetupColumn(heading);
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableHeadersRow();
        for (int slot = 0; slot < static_cast<int>(runs.size()); ++slot) {
            if (!runs[slot]) continue;
            const auto& r = *runs[slot];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushID(slot);
            if (ImGui::Selectable(algorithmNames[slot / 4], selected == slot, ImGuiSelectableFlags_SpanAllColumns)) {
                selected = slot;
                algorithm = slot / 4;
                mode = slot % 4;
            }
            ImGui::PopID();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(modeNames[slot % 4]);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(r.success() ? "Found" : "Blocked");
            ImGui::TableNextColumn(); ImGui::Text("%zu/%zu", r.order.size(), graph.labels.size());
            ImGui::TableNextColumn(); ImGui::Text("%d%s", r.used, r.success() ? "" : " (partial)");
            ImGui::TableNextColumn(); ImGui::Text("%d", r.checks);
            ImGui::TableNextColumn(); ImGui::Text("%.6f", r.milliseconds);
            for (int value : {r.mrvSelections, r.mrvTies, r.lcvEvaluations, r.lcvTies, r.lcvChanges}) {
                ImGui::TableNextColumn();
                ImGui::Text("%d", value);
            }
        }
        ImGui::EndTable();
    }
    const auto& active = *runs[selected];
    ImGui::TextWrapped("Assignment order: %s", orderText(active).c_str());
    if (!active.success())
        ImGui::TextWrapped("Blocked at %s by %s.", graph.labels[active.failedVertex].c_str(), blockersText(active).c_str());
    ImGui::SeparatorText("Export results");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##exportPath", &exportPath);
    if (ImGui::Button("Save CSV")) {
        std::ofstream output(exportPath, std::ios::noreplace);
        output << csv();
        output.close();
        exportMessage = output ? "Saved to " + std::filesystem::absolute(exportPath).string()
                               : "Could not save. Choose a new file in a writable directory.";
    }
    if (!exportMessage.empty()) ImGui::TextWrapped("%s", exportMessage.c_str());
}
} // namespace coloring
