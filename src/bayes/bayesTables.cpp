#include <bayes.hpp>

#include <algorithm>
#include <array>
#include <numeric>
#include <string>

#include <imgui.h>

namespace {
    constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders
        | ImGuiTableFlags_RowBg
        | ImGuiTableFlags_SizingStretchProp
        | ImGuiTableFlags_ScrollX
        | ImGuiTableFlags_ScrollY;

    void drawTableNote(const std::string& message) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", message.c_str());
        ImGui::PopStyleColor();
    }

    float tableInnerWidth(float minimumWidth) {
        return std::max(minimumWidth,
            ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize);
    }
}

void Bayes::drawDataView() {
    const float fontSize = ImGui::GetFontSize();
    const float tableHeight = std::clamp(ImGui::GetWindowSize().y * 0.28f,
        10.0f * fontSize, 16.0f * fontSize);

    ImGui::SeparatorText("Bins - Training counts");
    ImGui::TextDisabled("Training samples: %.0f", this->bayesClassifier->getRegisterTotal());
    drawTableNote("Counts include the original 80% training subset and people added this session.");
    this->drawBinsTable(tableHeight);

    ImGui::Spacing();
    ImGui::SeparatorText("Dataset");
    ImGui::TextDisabled("People: %zu", this->bayesClassifier->getDataSet().records.size());
    drawTableNote("All original records (training and test), plus people added this session.");
    this->drawDataSetTable(tableHeight);

    ImGui::Spacing();
    this->drawConfusionMatrix();
}

void Bayes::drawBinsTable(float height) {
    static const std::array<std::string, 3> ageLabels{"joven", "adulto", "mayor"};
    static const std::array<std::string, 5> incomeLabels{
        "muyBajo", "bajo", "medio", "alto", "muyAlto"
    };
    const auto& bins = this->bayesClassifier->getSampleBins();
    std::array<float, 4> columnWidths{
        ImGui::CalcTextSize("Age bin").x, ImGui::CalcTextSize("Income bin").x,
        ImGui::CalcTextSize("Camina").x, ImGui::CalcTextSize("Conduce").x
    };
    for(const auto& label : ageLabels)
        columnWidths[0] = std::max(columnWidths[0], ImGui::CalcTextSize(label.c_str()).x);
    for(const auto& label : incomeLabels)
        columnWidths[1] = std::max(columnWidths[1], ImGui::CalcTextSize(label.c_str()).x);
    for(float& width : columnWidths) width += 2.0f * ImGui::GetStyle().CellPadding.x + 2.0f;
    const float minimumWidth = std::accumulate(columnWidths.begin(), columnWidths.end(), 0.0f);

    if(ImGui::BeginTable("TrainingBins", 4, tableFlags, ImVec2(0.0f, height), tableInnerWidth(minimumWidth))) {
        ImGui::TableSetupColumn("Age bin", ImGuiTableColumnFlags_WidthStretch, columnWidths[0]);
        ImGui::TableSetupColumn("Income bin", ImGuiTableColumnFlags_WidthStretch, columnWidths[1]);
        ImGui::TableSetupColumn("Camina", ImGuiTableColumnFlags_WidthStretch, columnWidths[2]);
        ImGui::TableSetupColumn("Conduce", ImGuiTableColumnFlags_WidthStretch, columnWidths[3]);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for(std::size_t age = 0; age < bins.size(); ++age) {
            for(std::size_t income = 0; income < bins[age].size(); ++income) {
                const auto& counts = bins[age][income];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ageLabels[age].c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(incomeLabels[income].c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%d", counts[static_cast<std::size_t>(Category::camina)]);
                ImGui::TableNextColumn();
                ImGui::Text("%d", counts[static_cast<std::size_t>(Category::conduce)]);
            }
        }
        ImGui::EndTable();
    }
}

void Bayes::drawDataSetTable(float height) {
    const auto& records = this->bayesClassifier->getDataSet().records;
    if(records.empty()) {
        ImGui::TextWrapped("No people in the dataset. Add a person using the input menu.");
        return;
    }

    if(ImGui::BeginTable("DataSetRecords", 4, tableFlags, ImVec2(0.0f, height), tableInnerWidth(27.0f * ImGui::GetFontSize()))) {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Age", ImGuiTableColumnFlags_WidthStretch, 0.9f);
        ImGui::TableSetupColumn("Income", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthStretch, 1.3f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(records.size()));
        while(clipper.Step()) {
            for(int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const Person& person = records[static_cast<std::size_t>(row)];
                const std::string category = person.category == Category::camina ? "Camina" : "Conduce";
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", row + 1);
                ImGui::TableNextColumn();
                ImGui::Text("%.15g", person.age);
                ImGui::TableNextColumn();
                ImGui::Text("%.15g", person.income);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(category.c_str());
            }
        }
        ImGui::EndTable();
    }
}
