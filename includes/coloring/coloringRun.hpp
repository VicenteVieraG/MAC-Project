#pragma once

#include <coloringGraph.hpp>

#include <vector>

namespace coloring {
    /// Base vertex ordering and optional vertex (MRV) and color (LCV) heuristics.
    enum class Algorithm { Greedy, WelshPowell, MMI };
    enum class Mode { Basic, MRV, LCV, Both };
    inline constexpr const char* algorithmNames[]{"Greedy", "Welsh-Powell", "MMI"};
    inline constexpr const char* modeNames[]{"Basic", "MRV", "LCV", "MRV + LCV"};

    /// Assignment and metrics for one constructive run. Partial assignments survive failure.
    struct Run {
        Algorithm algorithm;
        Mode mode;
        std::vector<int> colors{}; // Palette indices, or -1 for an uncolored vertex.
        std::vector<int> order{};  // Successful assignments in order.
        int failedVertex = -1; // First blocked vertex, or -1 on success.
        int used = 0;
        int checks = 0; // Color legality lookups, including those used by MRV and LCV.
        int mrvSelections = 0;
        int mrvTies = 0;
        int lcvEvaluations = 0;
        int lcvTies = 0;
        int lcvChanges = 0;
        double milliseconds = 0;
        bool success() const { return failedVertex < 0; }
    };

    /// Color a validated graph using palette indices 0 through k-1 in priority order.
    /// Stops at the first blocked vertex. Throws std::invalid_argument for unsupported k.
    Run color(const Graph& graph, int k, Algorithm algorithm, Mode mode);

} // namespace coloring
