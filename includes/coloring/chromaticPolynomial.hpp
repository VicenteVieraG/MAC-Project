#pragma once

#include <coloringGraph.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace coloring {
    inline constexpr int maxExactVertices = 10;

    /// Exact polynomial and measurements for one deletion-contraction calculation.
    struct ExactResult {
        // Coefficients are indexed by degree, starting with the constant.
        std::vector<std::int64_t> coefficients;
        int chromaticNumber = 0;
        int calls = 0; // Includes the root, leaves and cache hits.
        double milliseconds = 0;
        std::string trace; // Call trace for graphs with up to four vertices.

        /// Count proper colorings with k labeled colors (1 through maxColors).
        std::int64_t evaluate(int k) const;
        /// Format the polynomial in descending powers of q.
        std::string formula() const;
    };

    /// Analyze a validated graph of 1 through maxExactVertices vertices.
    /// Throws std::runtime_error if the recursive call budget is exhausted.
    ExactResult analyze(const Graph& graph);

} // namespace coloring
