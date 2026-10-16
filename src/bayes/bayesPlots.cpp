#include <bayes.hpp>

#include <algorithm>
#include <array>
#include <string>

#include <imgui.h>
#include <implot.h>

namespace {
    ImPlotColormap confusionMatrixColormap() {
        const char* name = "ConfusionMatrixBlues";
        const ImPlotColormap existing = ImPlot::GetColormapIndex(name);
        if(existing != -1) return existing;

        constexpr ImVec4 colors[] = {
            {0.969f, 0.984f, 1.000f, 1.0f},
            {0.776f, 0.859f, 0.937f, 1.0f},
            {0.420f, 0.682f, 0.839f, 1.0f},
            {0.129f, 0.443f, 0.710f, 1.0f},
            {0.031f, 0.188f, 0.420f, 1.0f}
        };
        return ImPlot::AddColormap(name, colors, IM_ARRAYSIZE(colors), false);
    }
}

void Bayes::drawConfusionMatrix() {
    ImGui::SeparatorText("Confusion matrix");
    if(!this->evaluationResult.hasEvaluated()) {
        ImGui::TextWrapped("Evaluation unavailable. Load training data to see the confusion matrix.");
        return;
    }
    if(this->evaluationResult.getEvaluatedSampleCount() == 0) {
        ImGui::TextWrapped("No test samples available. Load a dataset with a test subset.");
        return;
    }

    ImGui::TextDisabled("Test samples: %zu", this->evaluationResult.getEvaluatedSampleCount());
    ImGui::TextWrapped("Evaluated on the original 20%% test subset.");

    // Row-major: actual categories down the rows, predictions across columns.
    const std::array counts{
        this->evaluationResult.getVP(), this->evaluationResult.getFN(),
        this->evaluationResult.getFP(), this->evaluationResult.getVN()
    };
    const int maxCount = std::max(1, *std::max_element(counts.begin(), counts.end()));
    const float fontSize = ImGui::GetFontSize();
    const ImVec2 padding = ImPlot::GetStyle().PlotPadding;
    const ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float tickWidth = ImGui::CalcTextSize(std::to_string(maxCount).c_str()).x;
    const float scaleWidth = 20.0f + 2.0f * padding.x
        + ImPlot::GetStyle().LabelPadding.x + std::max(tickWidth, ImGui::CalcTextSize("0.5").x);
    const float plotWidth = std::clamp(ImGui::GetContentRegionAvail().x
        - 2.0f * windowPadding.x - scaleWidth - spacing, 12.0f * fontSize, 30.0f * fontSize);
    // Account for the wider row labels so the cells themselves remain square.
    const float plotHeight = plotWidth + ImGui::GetTextLineHeight() - ImGui::CalcTextSize("Conduce").x;
    const float panelHeight = plotHeight + 2.0f * windowPadding.y + ImGui::GetStyle().ScrollbarSize;

    // Keep the reference's white chart surface within the app's dark theme.
    constexpr ImVec4 white{1.0f, 1.0f, 1.0f, 1.0f};
    constexpr ImVec4 ink{0.06f, 0.09f, 0.14f, 1.0f};
    ImGui::PushStyleColor(ImGuiCol_ChildBg, white);
    ImGui::PushStyleColor(ImGuiCol_Text, ink);
    if(ImGui::BeginChild("ConfusionMatrixPanel", ImVec2(0.0f, panelHeight),
        ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
        constexpr const char* labels[] = {"Camina", "Conduce"};
        constexpr double columns[] = {0.5, 1.5};
        constexpr double rows[] = {1.5, 0.5};
        constexpr ImPlotAxisFlags axisFlags = ImPlotAxisFlags_Lock
            | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks;

        ImPlot::PushColormap(confusionMatrixColormap());
        ImPlot::PushStyleColor(ImPlotCol_FrameBg, white);
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, white);
        ImPlot::PushStyleColor(ImPlotCol_PlotBorder, ink);
        ImPlot::PushStyleColor(ImPlotCol_AxisText, ink);
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(0.0f, 0.0f));

        const ImVec2 origin = ImGui::GetCursorScreenPos();
        if(ImPlot::BeginPlot("##ConfusionMatrix", ImVec2(plotWidth, plotHeight),
            ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs)) {
            ImPlot::SetupAxes("Prediction", "Actual", axisFlags | ImPlotAxisFlags_Opposite, axisFlags);
            ImPlot::SetupAxesLimits(0.0, 2.0, 0.0, 2.0, ImPlotCond_Always);
            ImPlot::SetupAxisTicks(ImAxis_X1, columns, 2, labels);
            ImPlot::SetupAxisTicks(ImAxis_Y1, rows, 2, labels);
            ImPlot::PlotHeatmap("Count", counts.data(), 2, 2, 0.0, maxCount, "%.0f",
                ImPlotPoint(0.0, 0.0), ImPlotPoint(2.0, 2.0));
            const ImVec2 plotPosition = ImPlot::GetPlotPos();
            const ImVec2 plotSize = ImPlot::GetPlotSize();
            ImPlot::EndPlot();

            // Align the color gradient with the cells, excluding axis labels.
            ImGui::SameLine();
            ImGui::SetCursorScreenPos(ImVec2(origin.x + plotWidth + spacing, plotPosition.y - padding.y));
            ImPlot::ColormapScale("##ConfusionMatrixScale", 0.0, maxCount,
                ImVec2(0.0f, plotSize.y + 2.0f * padding.y), "%g");
        }

        ImPlot::PopStyleVar();
        ImPlot::PopStyleColor(4);
        ImPlot::PopColormap();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
}
