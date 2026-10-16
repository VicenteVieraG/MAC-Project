#include <bayes.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

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

void Bayes::drawPlotsView() {
    const float fontSize = ImGui::GetFontSize();
    const float plotHeight = std::clamp(ImGui::GetContentRegionAvail().y * 0.65f,
        16.0f * fontSize, 26.0f * fontSize);

    ImGui::SeparatorText("Age and income");
    ImGui::TextDisabled("People: %zu", this->bayesClassifier->getDataSet().records.size());
    ImGui::TextWrapped("All records, including people added this session. Hover over a point for details.");
    this->drawScatterPlot(plotHeight);

    ImGui::Spacing();
    ImGui::SeparatorText("Decision regions");
    if(ImGui::BeginChild("DecisionRegionPlaceholder", ImVec2(0.0f, 10.0f * fontSize),
        ImGuiChildFlags_Borders))
        ImGui::TextWrapped("Space reserved for the decision-region plot.");
    ImGui::EndChild();
}

void Bayes::drawScatterPlot(float height) {
    const auto& records = this->bayesClassifier->getDataSet().records;
    if(records.empty()) {
        ImGui::TextWrapped("No people to plot. Add a person using the input menu.");
        return;
    }

    std::array<std::vector<ImPlotPoint>, 2> points;
    for(const Person& person : records)
        points[static_cast<std::size_t>(person.category)].emplace_back(person.age, person.income);

    // Include new records and predictions without resetting zoom on every frame.
    if(records.size() != this->scatterRecordCount || this->scatterNeedsFit)
        ImPlot::SetNextAxesToFit();
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(0.0f, 0.0f));
    if(ImPlot::BeginPlot("##AgeIncomeScatter", ImVec2(-1.0f, height), ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes("Age (years)", "Income");
        // Keep the legend clear of points, including predictions at the plot edges.
        ImPlotLegendFlags legendFlags = ImPlotLegendFlags_NoButtons | ImPlotLegendFlags_Outside;
        if(ImGui::GetContentRegionAvail().x >= 24.0f * ImGui::GetFontSize())
            legendFlags |= ImPlotLegendFlags_Horizontal;
        ImPlot::SetupLegend(ImPlotLocation_South, legendFlags);

        constexpr const char* labels[] = {"Camina", "Conduce"};
        constexpr ImVec4 colors[] = {
            {0.39f, 0.67f, 1.00f, 1.0f},
            {1.00f, 0.66f, 0.32f, 1.0f}
        };
        const float markerSize = std::max(4.0f, ImGui::GetFontSize() * 0.25f);
        for(std::size_t category = 0; category < points.size(); ++category) {
            ImPlotSpec spec;
            spec.Marker = category == 0 ? ImPlotMarker_Circle : ImPlotMarker_Diamond;
            spec.MarkerSize = markerSize;
            spec.LineColor = colors[category];
            ImPlot::PlotScatterG(labels[category], [](int index, void* data) {
                return static_cast<const ImPlotPoint*>(data)[index];
            }, points[category].data(), static_cast<int>(points[category].size()), spec);
        }

        const float predictionMarkerSize = markerSize * 2.0f;
        if(this->predictionResult) {
            ImPlotSpec spec;
            spec.Marker = ImPlotMarker_Asterisk;
            spec.MarkerSize = predictionMarkerSize;
            spec.LineWeight = 2.0f;
            spec.LineColor = ImVec4(0.78f, 0.52f, 1.00f, 1.0f);
            ImPlot::PlotScatter("Prediction", &this->predictionInput.age,
                &this->predictionInput.income, 1, spec);
        }

        if(ImPlot::IsPlotHovered()) {
            const ImVec2 mouse = ImGui::GetMousePos();
            const ImPlotRect limits = ImPlot::GetPlotLimits();
            const float hoverRadius = markerSize + 4.0f;
            float nearestDistanceSquared = hoverRadius * hoverRadius;
            const Person* hovered = nullptr;
            bool hoveringPrediction = false;
            // Use screen distance: age and income have very different scales.
            for(const Person& person : records) {
                if(!limits.Contains(person.age, person.income)) continue;
                const ImVec2 pixel = ImPlot::PlotToPixels(person.age, person.income);
                const float dx = mouse.x - pixel.x;
                const float dy = mouse.y - pixel.y;
                const float distanceSquared = dx * dx + dy * dy;
                if(distanceSquared < nearestDistanceSquared) {
                    nearestDistanceSquared = distanceSquared;
                    hovered = &person;
                }
            }
            if(this->predictionResult && limits.Contains(this->predictionInput.age, this->predictionInput.income)) {
                const ImVec2 pixel = ImPlot::PlotToPixels(this->predictionInput.age, this->predictionInput.income);
                const float dx = mouse.x - pixel.x;
                const float dy = mouse.y - pixel.y;
                const float distanceSquared = dx * dx + dy * dy;
                const float predictionHoverRadius = predictionMarkerSize + 4.0f;
                // Prefer the prediction when it overlaps an existing record exactly.
                if(distanceSquared < predictionHoverRadius * predictionHoverRadius
                    && (!hovered || distanceSquared <= nearestDistanceSquared)) {
                    hovered = &this->predictionInput;
                    hoveringPrediction = true;
                }
            }
            if(hovered) {
                ImPlot::PushPlotClipRect();
                ImPlot::GetPlotDrawList()->AddCircle(
                    ImPlot::PlotToPixels(hovered->age, hovered->income),
                    (hoveringPrediction ? predictionMarkerSize : markerSize) + 3.0f,
                    ImGui::GetColorU32(ImGuiCol_Text), 0, 2.0f);
                ImPlot::PopPlotClipRect();
                ImGui::BeginTooltip();
                if(hoveringPrediction)
                    ImGui::Text("Prediction: %s", *this->predictionResult == Category::camina ? "Camina" : "Conduce");
                ImGui::Text("Age: %.15g years", hovered->age);
                ImGui::Text("Income: %.15g", hovered->income);
                ImGui::EndTooltip();
            }
        }
        ImPlot::EndPlot();
        this->scatterRecordCount = records.size();
        this->scatterNeedsFit = false;
    }
    ImPlot::PopStyleVar();
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
