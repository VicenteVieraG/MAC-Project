#pragma once

#include <coloringGraph.hpp>
#include <chromaticPolynomial.hpp>
#include <coloringRun.hpp>

#include <array>
#include <optional>

namespace coloring {
    class ColoringView {
    public:
        void draw();
    private:
        void load(const std::string& text, const std::string& name);
        void loadExample(int index);
        void clearRuns();
        void drawInput();
        void drawWorkbench();
        void drawControls();
        void drawExact();
        void calculateExact();
        void drawDiagram();
        void drawComparison();
        void run(int algorithm, int mode);
        std::string csv() const;
        std::string orderText(const Run& result) const;
        std::string blockersText(const Run& result) const;

        bool initialized = false;
        Graph graph;
        std::string graphName, draft, filePath, inputError;
        int k = 3;
        std::string colorNames = "Coral Teal Gold";
        std::vector<std::string> palette{"Coral", "Teal", "Gold"};
        std::string paletteError, exactError;
        std::optional<ExactResult> exact;
        int algorithm = 0, mode = 0, selected = -1;
        std::array<std::optional<Run>, 12> runs;
        std::string exportPath = "coloring-results.csv", exportMessage;
    };
} // namespace coloring
