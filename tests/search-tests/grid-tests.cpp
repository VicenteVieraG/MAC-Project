#include <catch2/catch_test_macros.hpp>

#include <dijkstra.hpp>
#include <grid.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
    constexpr double inf = std::numeric_limits<double>::infinity();
}

TEST_CASE("Grid side length creates a square with only four-direction unit-cost edges", "[search][grid]") {
    const Grid grid(3);
    const std::vector<std::vector<std::size_t>> neighbors{
        {1, 3}, {0, 2, 4}, {1, 5},
        {0, 4, 6}, {1, 3, 5, 7}, {2, 4, 8},
        {3, 7}, {4, 6, 8}, {5, 7}
    };
    REQUIRE(grid.matrix().size() == 9);
    for(std::size_t from = 0; from < 9; ++from) {
        REQUIRE(grid.matrix()[from].size() == 9);
        for(std::size_t to = 0; to < 9; ++to) {
            const auto connected = std::find(neighbors[from].begin(), neighbors[from].end(), to)
                != neighbors[from].end();
            REQUIRE(grid.matrix()[from][to] == (from == to ? 0.0 : connected ? 1.0 : inf));
        }
    }
    REQUIRE(grid.nodeId(1, 0) == 3);
    REQUIRE(grid.nodeId(1, 2) == 5);
    REQUIRE(grid.positions().size() == 9);
    REQUIRE(grid.positions()[0].row == 0);
    REQUIRE(grid.positions()[0].column == 0);
    REQUIRE(grid.positions()[3].row == 1);
    REQUIRE(grid.positions()[3].column == 0);
    REQUIRE(grid.positions()[5].row == 1);
    REQUIRE(grid.positions()[5].column == 2);
    REQUIRE(grid.nodeId(2, 2) == 8);
    REQUIRE(grid.positions()[8].row == 2);
    REQUIRE(grid.positions()[8].column == 2);
    REQUIRE_THROWS_AS(grid.nodeId(3, 0), std::out_of_range);
    REQUIRE_THROWS_AS(grid.nodeId(0, 3), std::out_of_range);
}

TEST_CASE("Grid obstacles remove movement edges in both directions", "[search][grid]") {
    Grid grid(3);
    grid.setBlocked(4);
    for(std::size_t node = 0; node < 9; ++node) {
        REQUIRE(grid.matrix()[4][node] == (node == 4 ? 0.0 : inf));
        REQUIRE(grid.matrix()[node][4] == (node == 4 ? 0.0 : inf));
    }
    REQUIRE(grid.matrix()[0][1] == 1.0);
    REQUIRE(grid.matrix()[0][3] == 1.0);

    // Two blocked orthogonal neighbors cannot be bypassed by a diagonal.
    Grid corner(2);
    corner.setBlocked(1);
    corner.setBlocked(2);
    for(const auto type : {Heuristics::Type::None, Heuristics::Type::Manhattan, Heuristics::Type::Euclidean}) {
        Dijkstra search(corner.matrix(), 0, 3, corner.positions(), type);
        REQUIRE_FALSE(search.run().found);
        REQUIRE(search.result().exploredNodes == 1);
    }
}

TEST_CASE("Grid handles single-cell and larger side lengths", "[search][grid]") {
    Grid single(1);
    REQUIRE(single.matrix() == Grid::Matrix{{0.0}});
    REQUIRE(single.nodeId(0, 0) == 0);
    Dijkstra same(single.matrix(), 0, 0);
    REQUIRE(same.run().totalCost == 0.0);
    single.setBlocked(0);
    single.setBlocked(0, false);
    REQUIRE(single.matrix() == Grid::Matrix{{0.0}});

    for(const std::size_t side : {2, 3, 4, 5, 10}) {
        const auto count = side * side;
        const Grid grid(side);
        REQUIRE(grid.matrix().size() == count);
        REQUIRE(grid.positions().size() == count);
        REQUIRE(grid.nodeId(side - 1, side - 1) == count - 1);
        REQUIRE(grid.positions().back().row == static_cast<double>(side - 1));
        REQUIRE(grid.positions().back().column == static_cast<double>(side - 1));
        REQUIRE_THROWS_AS(grid.nodeId(side, 0), std::out_of_range);
        REQUIRE_THROWS_AS(grid.nodeId(0, side), std::out_of_range);
        Dijkstra search(grid.matrix(), 0, count - 1);
        REQUIRE(search.run(false).totalCost == static_cast<double>(2 * (side - 1)));
    }
}

TEST_CASE("Grid rejects zero and overflowing side lengths before allocation", "[search][grid]") {
    REQUIRE_THROWS_AS(Grid(0), std::invalid_argument);
    // This power of two is the first side length whose square overflows size_t.
    const auto firstOverflowSide = std::size_t{1} << ((std::numeric_limits<std::size_t>::digits + 1) / 2);
    REQUIRE_THROWS_AS(Grid(firstOverflowSide), std::overflow_error);
    REQUIRE_THROWS_AS(Grid(std::numeric_limits<std::size_t>::max()), std::overflow_error);
}

TEST_CASE("Grid obstacle edits restore only legal edges to open neighbors", "[search][grid]") {
    Grid grid(3);
    const auto open = grid.matrix();
    grid.setBlocked(4);
    grid.setBlocked(1);
    grid.setBlocked(4, false);
    REQUIRE(grid.matrix()[4][1] == inf);
    REQUIRE(grid.matrix()[1][4] == inf);
    for(const std::size_t neighbor : {3, 5, 7}) {
        REQUIRE(grid.matrix()[4][neighbor] == 1.0);
        REQUIRE(grid.matrix()[neighbor][4] == 1.0);
    }
    REQUIRE(grid.matrix()[4][0] == inf); // No diagonal appears when unblocking.
    REQUIRE(grid.matrix()[4][4] == 0.0);
    grid.setBlocked(1, false);
    REQUIRE(grid.matrix() == open);
    for(std::size_t node = 0; node < 9; ++node) grid.setBlocked(node);
    for(std::size_t node = 0; node < 9; ++node) {
        grid.setBlocked(node, false);
        grid.setBlocked(node, false); // Repeated edits are harmless.
    }
    REQUIRE(grid.matrix() == open);
    REQUIRE_THROWS_AS(grid.setBlocked(9), std::out_of_range);
    REQUIRE_THROWS_AS(grid.setBlocked(std::numeric_limits<std::size_t>::max(), false), std::out_of_range);
    REQUIRE(grid.matrix() == open);
}

TEST_CASE("Grid searches agree on unit-cost obstacle detours", "[search][grid]") {
    // A wall below the top-middle cell forces the path around the bottom row.
    Grid grid(3);
    grid.setBlocked(grid.nodeId(0, 1));
    grid.setBlocked(grid.nodeId(1, 1));
    Dijkstra baseline(grid.matrix(), grid.nodeId(0, 0), grid.nodeId(0, 2));
    const auto expected = baseline.run(false);
    REQUIRE(expected.totalCost == 6.0);
    REQUIRE(expected.path == std::vector<Dijkstra::NodeId>{0, 3, 6, 7, 8, 5, 2});
    for(const auto type : {Heuristics::Type::Manhattan, Heuristics::Type::Euclidean}) {
        Dijkstra search(grid.matrix(), 0, 2, grid.positions(), type);
        const auto& result = search.run(false);
        REQUIRE(result.found);
        REQUIRE(result.path == expected.path);
        REQUIRE(result.totalCost == expected.totalCost);
        REQUIRE(result.totalCost == static_cast<double>(result.path.size() - 1));
    }
}
