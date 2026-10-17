#include <bayes.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>

namespace {
    constexpr const char* categoryLabels[] = {"Camina", "Conduce"};
    constexpr ImVec4 categoryColors[] = {
        {0.39f, 0.67f, 1.00f, 1.0f},
        {1.00f, 0.66f, 0.32f, 1.0f}
    };
    constexpr ImVec4 predictionColor{0.78f, 0.52f, 1.00f, 1.0f};

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
    this->drawDecisionRegions(plotHeight);
}

void Bayes::drawScatterPlot(float height) {
    const auto& records = this->bayesClassifier->getDataSet().records;
    if(records.empty()) {
        ImGui::TextWrapped("No people to plot. Add a person using the input menu.");
        return;
    }

    ImGui::TextWrapped("Scroll to zoom, drag to pan. Ctrl+click to predict here, then drag the purple star.");

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
        ImPlot::SetupFinish();

        const bool plotHovered = ImPlot::IsPlotHovered();
        const ImPlotPoint plotMouse = ImPlot::GetPlotMousePos();
        const bool validMouse = std::isfinite(plotMouse.x) && std::isfinite(plotMouse.y)
            && plotMouse.x > 0.0 && plotMouse.y >= 0.0;
        if(plotHovered && validMouse && ImGui::GetIO().KeyCtrl
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            this->predictionInput.age = plotMouse.x;
            this->predictionInput.income = plotMouse.y;
            this->predictionResult = this->bayesClassifier->predict(this->predictionInput);
            this->decisionNeedsFit = true;
        }

        const float markerSize = std::max(4.0f, ImGui::GetFontSize() * 0.25f);
        for(std::size_t category = 0; category < points.size(); ++category) {
            ImPlotSpec spec;
            spec.Marker = category == 0 ? ImPlotMarker_Circle : ImPlotMarker_Diamond;
            spec.MarkerSize = markerSize;
            spec.LineColor = categoryColors[category];
            ImPlot::PlotScatterG(categoryLabels[category], [](int index, void* data) {
                return static_cast<const ImPlotPoint*>(data)[index];
            }, points[category].data(), static_cast<int>(points[category].size()), spec);
        }

        const float predictionMarkerSize = markerSize * 2.0f;
        bool predictionHovered = false;
        bool predictionHeld = false;
        if(this->predictionResult) {
            if(ImPlot::DragPoint(0, &this->predictionInput.age, &this->predictionInput.income,
                predictionColor, markerSize * 1.5f, ImPlotDragToolFlags_Delayed,
                nullptr, &predictionHovered, &predictionHeld)) {
                this->predictionInput.age = std::max(std::numeric_limits<double>::epsilon(), this->predictionInput.age);
                this->predictionInput.income = std::max(0.0, this->predictionInput.income);
                // Refit the other plot while preserving this plot's view during dragging.
                this->decisionNeedsFit = true;
            }
            this->predictionResult = this->bayesClassifier->predict(this->predictionInput);
            ImPlotSpec spec;
            spec.Marker = ImPlotMarker_Asterisk;
            spec.MarkerSize = predictionMarkerSize;
            spec.LineWeight = 2.0f;
            spec.LineColor = predictionColor;
            spec.MarkerLineColor = predictionColor;
            ImPlot::PlotScatter("Prediction", &this->predictionInput.age,
                &this->predictionInput.income, 1, spec);
        }

        if(plotHovered || predictionHovered || predictionHeld) {
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
            if(predictionHovered || predictionHeld) {
                hovered = &this->predictionInput;
                hoveringPrediction = true;
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

void Bayes::drawDecisionRegions(float height) {
    if(this->bayesClassifier->getRegisterTotal() == 0.0) {
        ImGui::TextWrapped("No decision regions yet. Add a person to train the model.");
        return;
    }

    ImGui::TextWrapped("Background colors show the current prediction; points show known categories. "
        "Regions update when the model changes.");
    ImGui::TextWrapped("Scroll to zoom, drag to pan. Ctrl+click to predict here, then drag the purple point.");
    ImGui::PushID("DecisionRegions");
    if(ImGui::Button("Reset view")) this->decisionNeedsFit = true;
    ImGui::SameLine();
    ImGui::Checkbox("Show people", &this->decisionShowPeople);

    const auto& ageBounds = BayesClassifier::ageBinBoundaries;
    const auto& incomeBounds = BayesClassifier::incomeBinBoundaries;
    const auto& records = this->bayesClassifier->getDataSet().records;
    double ageMax = ageBounds.back() * 1.25;
    double incomeMax = incomeBounds.back() * 1.25;
    const auto includePoint = [&](const Person& person) {
        if(std::isfinite(person.age)) ageMax = std::max(ageMax, person.age);
        if(std::isfinite(person.income)) incomeMax = std::max(incomeMax, person.income);
    };
    std::array<std::vector<ImPlotPoint>, 2> points;
    for(const Person& person : records) {
        includePoint(person);
        if(this->decisionShowPeople)
            points[static_cast<std::size_t>(person.category)].emplace_back(person.age, person.income);
    }
    if(this->predictionResult) includePoint(this->predictionInput);
    // Leave room around the most distant point without overflowing large inputs.
    if(ageMax < std::numeric_limits<double>::max() / 1.05) ageMax *= 1.05;
    if(incomeMax < std::numeric_limits<double>::max() / 1.05) incomeMax *= 1.05;

    ImPlot::PushStyleVar(ImPlotStyleVar_PlotMinSize, ImVec2(0.0f, 0.0f));
    if(ImPlot::BeginPlot("##DecisionRegions", ImVec2(-1.0f, height), ImPlotFlags_NoMouseText)) {
        ImPlot::SetupAxes("Age (years)", "Income");
        ImPlot::SetupAxesLimits(0.0, ageMax, 0.0, incomeMax,
            this->decisionNeedsFit ? ImPlotCond_Always : ImPlotCond_Once);
        ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0, std::numeric_limits<double>::max());
        ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.0, std::numeric_limits<double>::max());
        ImPlotLegendFlags legendFlags = ImPlotLegendFlags_NoButtons | ImPlotLegendFlags_Outside;
        if(ImGui::GetContentRegionAvail().x >= 24.0f * ImGui::GetFontSize())
            legendFlags |= ImPlotLegendFlags_Horizontal;
        ImPlot::SetupLegend(ImPlotLocation_South, legendFlags);
        ImPlot::SetupFinish();

        const ImPlotRect limits = ImPlot::GetPlotLimits();
        const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        const bool hovered = ImPlot::IsPlotHovered();
        const bool validMouse = std::isfinite(mouse.x) && std::isfinite(mouse.y)
            && mouse.x > 0.0 && mouse.y >= 0.0;
        if(hovered && validMouse && ImGui::GetIO().KeyCtrl
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            this->predictionInput.age = mouse.x;
            this->predictionInput.income = mouse.y;
            this->predictionResult = this->bayesClassifier->predict(this->predictionInput);
            this->scatterNeedsFit = true;
        }

        // The classifier is constant within each bin. Draw the exact, unequal
        // intervals and extend the open-ended bins to the current viewport.
        // Query the model every frame so partialFit also updates the colors.
        ImPlot::PushPlotClipRect();
        ImDrawList* drawList = ImPlot::GetPlotDrawList();
        for(std::size_t age = 0; age <= ageBounds.size(); ++age) {
            const double xMin = std::max(limits.X.Min, age == 0 ? 0.0 : ageBounds[age - 1]);
            const double xMax = age < ageBounds.size() ? std::min(limits.X.Max, ageBounds[age]) : limits.X.Max;
            if(xMin >= xMax) continue;
            for(std::size_t income = 0; income <= incomeBounds.size(); ++income) {
                const double yMin = std::max(limits.Y.Min, income == 0 ? 0.0 : incomeBounds[income - 1]);
                const double yMax = income < incomeBounds.size() ? std::min(limits.Y.Max, incomeBounds[income]) : limits.Y.Max;
                if(yMin >= yMax) continue;
                const Person sample{std::midpoint(xMin, xMax), std::midpoint(yMin, yMax), Category::camina};
                const Category prediction = this->bayesClassifier->predict(sample);
                ImVec4 color = categoryColors[static_cast<std::size_t>(prediction)];
                color.w = 0.26f;
                const ImVec2 first = ImPlot::PlotToPixels(xMin, yMin);
                const ImVec2 second = ImPlot::PlotToPixels(xMax, yMax);
                const ImVec2 topLeft{std::min(first.x, second.x), std::min(first.y, second.y)};
                const ImVec2 bottomRight{std::max(first.x, second.x), std::max(first.y, second.y)};
                drawList->AddRectFilled(topLeft, bottomRight, ImGui::GetColorU32(color));
                drawList->AddRect(topLeft, bottomRight, ImGui::GetColorU32(ImGuiCol_Text, 0.12f));
            }
        }
        ImPlot::PopPlotClipRect();

        // Invisible anchors keep double-click fitting useful with people hidden.
        const double domainAge[]{0.0, ageMax};
        const double domainIncome[]{0.0, incomeMax};
        ImPlotSpec domainSpec;
        domainSpec.Flags = ImPlotItemFlags_NoLegend;
        domainSpec.LineColor = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        ImPlot::PlotLine("##Domain", domainAge, domainIncome, 2, domainSpec);

        const float markerSize = std::max(4.0f, ImGui::GetFontSize() * 0.25f);
        for(std::size_t category = 0; category < points.size(); ++category) {
            ImPlotSpec spec;
            spec.Marker = category == 0 ? ImPlotMarker_Circle : ImPlotMarker_Diamond;
            spec.MarkerSize = markerSize;
            spec.LineColor = categoryColors[category];
            spec.MarkerLineColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            ImPlot::PlotScatterG(categoryLabels[category], [](int index, void* data) {
                return static_cast<const ImPlotPoint*>(data)[index];
            }, points[category].data(), static_cast<int>(points[category].size()), spec);
        }

        bool probeHovered = false;
        bool probeHeld = false;
        if(this->predictionResult) {
            if(ImPlot::DragPoint(0, &this->predictionInput.age, &this->predictionInput.income,
                predictionColor, markerSize * 1.5f, ImPlotDragToolFlags_Delayed,
                nullptr, &probeHovered, &probeHeld)) {
                this->predictionInput.age = std::max(std::numeric_limits<double>::epsilon(), this->predictionInput.age);
                this->predictionInput.income = std::max(0.0, this->predictionInput.income);
                this->scatterNeedsFit = true;
            }
            this->predictionResult = this->bayesClassifier->predict(this->predictionInput);
            ImPlotSpec spec;
            spec.Marker = ImPlotMarker_Asterisk;
            spec.MarkerSize = markerSize * 2.0f;
            spec.LineWeight = 2.0f;
            spec.LineColor = predictionColor;
            spec.MarkerLineColor = predictionColor;
            ImPlot::PlotScatter("Prediction", &this->predictionInput.age, &this->predictionInput.income, 1, spec);
        }

        if((hovered && validMouse) || probeHovered || probeHeld) {
            const Person sample = probeHovered || probeHeld ? this->predictionInput
                : Person{mouse.x, mouse.y, Category::camina};
            const Category prediction = this->bayesClassifier->predict(sample);
            const auto age = static_cast<std::size_t>(std::upper_bound(ageBounds.begin(), ageBounds.end(), sample.age) - ageBounds.begin());
            const auto income = static_cast<std::size_t>(std::upper_bound(incomeBounds.begin(), incomeBounds.end(), sample.income) - incomeBounds.begin());
            const auto& counts = this->bayesClassifier->getSampleBins()[age][income];
            ImGui::BeginTooltip();
            ImGui::Text("Prediction: %s", categoryLabels[static_cast<std::size_t>(prediction)]);
            ImGui::Text("Age: %.6g years | Income: %.6g", sample.age, sample.income);
            ImGui::Text("Training in this bin: Camina %d | Conduce %d", counts[0], counts[1]);
            ImGui::EndTooltip();
        }

        ImPlot::EndPlot();
        this->decisionNeedsFit = false;
    }
    ImPlot::PopStyleVar();
    ImGui::PopID();
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
