#pragma once

#include <dijkstra.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

/**
 * @brief Compare Dijkstra, Euclidean A*, and Manhattan A* without playback.
 *
 * The graph file format contains weights but no coordinates. Nodes therefore
 * receive artificial, row-major positions on a near-square grid. A common scale
 * derived from all finite directed edges makes both geometric heuristics
 * consistent with the weights. These estimates describe this synthetic layout,
 * not physical distances in the input graph. A zero-cost edge between distinct
 * nodes reduces the scale to zero, so all three searches behave as Dijkstra.
 */
class SearchBenchmark {
    public:
        using Matrix = Dijkstra::Matrix;

        struct Measurement {
            Heuristics::Type heuristic = Heuristics::Type::None;
            double milliseconds = 0.0;
            Dijkstra::SearchResult result;
        };

        /**
         * @brief Validate and own a graph, selecting nodes 0 and N-1 as endpoints.
         * @note Does not execute a search. A successful load clears measurements;
         *       a failed load preserves the entire previous benchmark state.
         * @throws std::invalid_argument For graph inputs rejected by Dijkstra.
         */
        void load(Matrix graph);

        /// Return false for an invalid node or before loading. Changes clear results.
        bool setStart(std::size_t node);
        bool setGoal(std::size_t node);

        /**
         * @brief Run all three searches with snapshot recording disabled.
         *
         * Only Dijkstra::run(false) is measured with a steady clock. Graph copies,
         * constructors, validation, and geometric estimates precede the timers;
         * copying each completed result follows its timer. Results are replaced
         * only after all three searches complete successfully.
         * @throws std::logic_error Before loading a graph.
         * @throws std::overflow_error If a search cost or priority overflows.
         */
        void compare();

        [[nodiscard]] bool loaded() const;
        [[nodiscard]] bool hasResults() const;
        [[nodiscard]] std::size_t nodeCount() const;
        /// Endpoints are zero before loading a graph.
        [[nodiscard]] std::size_t start() const;
        [[nodiscard]] std::size_t goal() const;
        /// Cost per synthetic grid unit; zero before load or with no useful geometry.
        [[nodiscard]] double geometryScale() const;

        /// Dijkstra, Euclidean, Manhattan order; throws before a completed comparison.
        [[nodiscard]] const std::array<Measurement, 3>& results() const;

    private:
        Matrix graph;
        std::vector<Heuristics::Position> positions;
        std::size_t startNode = 0;
        std::size_t goalNode = 0;
        double scale = 0.0;
        std::optional<std::array<Measurement, 3>> measurements;
};
