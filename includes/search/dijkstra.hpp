#pragma once

#include <heap.hpp>

#include <limits>
#include <optional>
#include <vector>

/**
 * @brief Dijkstra shortest-path search with optional snapshots for playback.
 *
 * Each instance owns its input matrix and latest successful run. The graph is
 * directed: matrix[u][v] is the cost from u to v. Positive infinity means no
 * edge; zero is a valid edge weight. No GUI or playback controls are involved.
 */
class Dijkstra {
    public:
        using NodeId = Heap::NodeId;
        using Matrix = std::vector<std::vector<double>>;

        /// @brief An owning copy of search state, indexed by node ID.
        struct SearchStep {
            /// Start node in the initial snapshot; otherwise the most recently popped node.
            NodeId current{};
            /// Tentative costs; only visited nodes have finalized costs.
            std::vector<double> distances;
            /// Default-initialized IDs; meaningful only for discovered nodes other than start.
            std::vector<NodeId> predecessors;
            std::vector<bool> visited;
            /// Frontier priorities are the corresponding entries in distances.
            std::vector<bool> inFrontier;
        };

        struct SearchResult {
            bool found = false;
            /// Infinity, with an empty path, when the goal is unreachable.
            double totalCost = std::numeric_limits<double>::infinity();
            std::vector<NodeId> path;
            /// Number of popped/finalized nodes, including the goal when found.
            std::size_t exploredNodes = 0;
            /// Initial state followed by one snapshot per popped node.
            std::vector<SearchStep> steps;
        };

        /**
         * @brief Own and validate a graph and its start/goal nodes.
         * @throws std::invalid_argument For an empty/nonsquare matrix, negative
         *         weights (including negative infinity), or NaN weights.
         * @throws std::out_of_range For a start or goal outside the matrix.
         */
        Dijkstra(Matrix graph, NodeId start, NodeId goal);

        /**
         * @brief Run ordinary Dijkstra to completion, replacing the previous result.
         * @param recordSteps Save independent snapshots when true (the default).
         * @return The stored result; valid until the next successful run or destruction.
         * @throws std::overflow_error If a considered finite path cost overflows.
         * @note Repeated runs start fresh. A failed run preserves the previous result.
         *       Without recording, steps is empty. The goal is accepted only when
         *       popped, and its outgoing edges are not expanded. Equal-cost routes
         *       keep their first predecessor; the heap breaks ties by node ID.
         */
        const SearchResult& run(bool recordSteps = true);

        /// @brief Read the latest result; throws std::logic_error before a completed run.
        [[nodiscard]] const SearchResult& result() const;

    private:
        Matrix graph;
        NodeId start{};
        NodeId goal{};
        std::optional<SearchResult> lastResult;
};
