#pragma once

#include <coloringGraph.hpp>
#include <chromaticPolynomial.hpp>
#include <coloringRun.hpp>

#include <array>
#include <cstdint>
#include <optional>

namespace coloring {
    class ColoringView {
    public:
        void draw();
    private:
        struct PaletteColor {
            std::string name;
            std::uint32_t swatch;
        };
        static std::vector<PaletteColor> makePalette(int count);
        std::string paletteText() const;
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
        std::vector<PaletteColor> palette = makePalette(k);
        std::string exactError;
        std::optional<ExactResult> exact;
        int algorithm = 0, mode = 0, selected = -1;
        std::array<std::optional<Run>, 12> runs;
        std::string exportPath = "coloring-results.csv", exportMessage;
    };
} // namespace coloring
