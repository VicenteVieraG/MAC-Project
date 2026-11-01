#pragma once

/// Geometry uses map coordinates, independently of where nodes are drawn.
namespace Heuristics {
    enum class Type { None, Manhattan, Euclidean };

    struct Position {
        double row = 0.0;
        double column = 0.0;
    };

    /// Sum of absolute coordinate differences. Coordinates must be finite.
    [[nodiscard]] double manhattan(Position node, Position goal);

    /// Straight-line distance, computed with hypot to avoid squared overflow.
    [[nodiscard]] double euclidean(Position node, Position goal);

    /**
     * @brief Estimate remaining cost; None returns zero.
     * @throws std::invalid_argument For nonfinite coordinates or an invalid type.
     * @throws std::overflow_error If a distance is not finite.
     * @note Each horizontal or vertical grid move costs 1, so distance directly
     *       estimates cost. Both heuristics are consistent on this grid.
     */
    [[nodiscard]] double estimate(Type type, Position node, Position goal);
}
