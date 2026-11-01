#pragma once

#include <heuristics.hpp>

#include <cstddef>
#include <vector>

/**
 * @brief A square map with unit-cost moves up, down, left, and right.
 *
 * Node IDs are row * sideLength + column. Obstacles retain their IDs and positions
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

        /**
         * @brief Block or unblock a node, updating its movement edges in both directions.
         * @note Unblocking restores only connections to unblocked orthogonal neighbors.
         * @throws std::out_of_range If node is outside the grid.
         */
        void setBlocked(std::size_t node, bool blocked = true);

        /// Unit-cost movement edges, zero diagonal, and positive infinity elsewhere.
        [[nodiscard]] const Matrix& matrix() const noexcept { return this->weights; }

        /// Map coordinates indexed by node ID, independent of screen placement.
        [[nodiscard]] const std::vector<Heuristics::Position>& positions() const noexcept {
            return this->coordinates;
        }

        /// Convert a cell to its node ID; throws std::out_of_range outside the map.
        [[nodiscard]] std::size_t nodeId(std::size_t row, std::size_t column) const;

    private:
        std::size_t sideLength;
        Matrix weights;
        std::vector<Heuristics::Position> coordinates;
        std::vector<bool> obstacles;
};
