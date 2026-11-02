#pragma once

#include <heuristics.hpp>

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

/**
 * @brief A compact map with unit-cost moves up, down, left, and right.
 *
 * Node IDs are row * columns() + column. Obstacles retain their IDs and positions
 * but have no movement edges. There are no diagonal edges or configurable costs.
 * Edit obstacles through setBlocked(); its matrix and positions are read-only.
 */
class Grid {
    public:
        using Matrix = std::vector<std::vector<double>>;

        /**
         * @brief Create an open square grid with sideLength * sideLength cells.
         * @param sideLength Number of cells along each side; e.g. 3 creates a 3 x 3 grid.
         * @throws std::invalid_argument If sideLength is zero.
         * @throws std::overflow_error If the total node count exceeds the size_t range.
         */
        explicit Grid(std::size_t sideLength);

        /// Exactly count cells in near-square rows, possibly with a partial last row.
        /// Throws std::invalid_argument for zero cells.
        [[nodiscard]] static Grid fromCellCount(std::size_t count);
        [[nodiscard]] std::size_t size() const noexcept { return this->obstacles.size(); }
        [[nodiscard]] std::size_t rows() const noexcept { return this->rowCount; }
        [[nodiscard]] std::size_t columns() const noexcept { return this->columnCount; }
        [[nodiscard]] bool isBlocked(std::size_t node) const;
        /// Open orthogonal neighbors in node-ID order; absent entries equal size().
        [[nodiscard]] std::array<std::size_t, 4> neighbors(std::size_t node) const;

        /**
         * @brief Block or unblock a node, updating its movement edges in both directions.
         * @note Unblocking restores only connections to unblocked orthogonal neighbors.
         * @throws std::out_of_range If node is outside the grid.
         */
        void setBlocked(std::size_t node, bool blocked = true);

        /// Replace obstacles with a seeded random maze, keeping both endpoints open
        /// and connected. Uses iterative carving with O(size()) time and storage.
        /// Throws std::out_of_range for an invalid endpoint before changing the map.
        void generateMaze(std::size_t start, std::size_t goal, std::uint32_t seed);

        /// Lazy compatibility matrix: unit edges, zero diagonal, infinity elsewhere.
        /// Costs O(size() squared) memory; visualization uses neighbors() instead.
        [[nodiscard]] const Matrix& matrix() const;

        /// Map coordinates indexed by node ID, independent of screen placement.
        [[nodiscard]] const std::vector<Heuristics::Position>& positions() const noexcept {
            return this->coordinates;
        }

        /// Convert a cell to its node ID; throws std::out_of_range outside the map.
        [[nodiscard]] std::size_t nodeId(std::size_t row, std::size_t column) const;

    private:
        Grid(std::size_t count, std::size_t columns);
        std::size_t rowCount;
        std::size_t columnCount;
        mutable Matrix weights;
        std::vector<Heuristics::Position> coordinates;
        std::vector<bool> obstacles;
};
