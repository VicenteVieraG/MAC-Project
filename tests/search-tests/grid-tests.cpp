#include <catch2/catch_test_macros.hpp>

#include <dijkstra.hpp>
#include <grid.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
    constexpr double inf = std::numeric_limits<double>::infinity();

    std::vector<bool> obstacleMask(const Grid& grid) {
        std::vector<bool> result(grid.size());
        for(std::size_t node = 0; node < grid.size(); ++node) result[node] = grid.isBlocked(node);
        return result;
    }

    std::vector<bool> reachableCells(const Grid& grid, std::size_t start) {
        std::vector<bool> reached(grid.size(), false);
        std::vector<std::size_t> pending{start};
        reached[start] = true;
        for(std::size_t index = 0; index < pending.size(); ++index) {
            for(const auto neighbor : grid.neighbors(pending[index])) {
                if(neighbor == grid.size() || reached[neighbor]) continue;
                reached[neighbor] = true;
                pending.push_back(neighbor);
            }
        }
        return reached;
    }
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

TEST_CASE("Random mazes connect custom endpoints and every open cell across grid shapes", "[search][grid][maze]") {
    // Consecutive counts cover odd/even dimensions, partial last rows, and tiny maps.
    for(std::size_t count = 1; count <= 100; ++count) {
        for(const std::uint32_t seed : {0, 1, 42}) {
            for(const auto start : {std::size_t{0}, count / 2, count - 1}) {
                const auto goal = (start + count / 2) % count;
                CAPTURE(count, seed, start, goal);
                auto grid = Grid::fromCellCount(count);
                const auto rows = grid.rows(), columns = grid.columns();
                for(std::size_t node = 0; node < count; ++node) grid.setBlocked(node);
                grid.generateMaze(start, goal, seed);
                REQUIRE(grid.size() == count);
                REQUIRE(grid.rows() == rows);
                REQUIRE(grid.columns() == columns);
                REQUIRE_FALSE(grid.isBlocked(start));
                REQUIRE_FALSE(grid.isBlocked(goal));
                const auto reached = reachableCells(grid, start);
                REQUIRE(reached[goal]);
                for(std::size_t node = 0; node < count; ++node) {
                    REQUIRE(reached[node] == !grid.isBlocked(node));
                    REQUIRE(grid.positions()[node].row == static_cast<double>(node / columns));
                    REQUIRE(grid.positions()[node].column == static_cast<double>(node % columns));
                }
                if(count >= 30) REQUIRE(std::count(reached.begin(), reached.end(), false) > 0);
            }
        }
    }
}

TEST_CASE("Random mazes reproduce a seed and replace earlier obstacles", "[search][grid][maze]") {
    auto grid = Grid::fromCellCount(95);
    grid.generateMaze(7, 94, 123);
    const auto first = obstacleMask(grid);
    for(std::size_t node = 0; node < grid.size(); ++node) grid.setBlocked(node, false);
    grid.generateMaze(7, 94, 123);
    REQUIRE(obstacleMask(grid) == first);
    grid.generateMaze(7, 94, 456);
    REQUIRE(obstacleMask(grid) != first);
}

TEST_CASE("Random maze corridors form a connected tree before endpoint extensions", "[search][grid][maze]") {
    Grid grid(21);
    grid.generateMaze(0, grid.size() - 1, 987);
    const auto reached = reachableCells(grid, 0);
    std::size_t openCells = 0, directedEdges = 0;
    for(std::size_t node = 0; node < grid.size(); ++node) {
        if(grid.isBlocked(node)) continue;
        REQUIRE(reached[node]);
        ++openCells;
        for(const auto neighbor : grid.neighbors(node))
            if(neighbor != grid.size()) ++directedEdges;
    }
    REQUIRE(openCells > grid.size() / 3);
    REQUIRE(openCells < grid.size() * 2 / 3);
    REQUIRE(directedEdges == 2 * (openCells - 1));
}

TEST_CASE("Random maze generation updates an existing compatibility matrix in place", "[search][grid][maze]") {
    Grid grid(5);
    const auto& matrix = grid.matrix();
    const auto* firstRow = matrix.front().data();
    grid.generateMaze(6, 24, 10);
    REQUIRE(&matrix == &grid.matrix());
    REQUIRE(matrix.front().data() == firstRow);
    for(std::size_t from = 0; from < grid.size(); ++from) {
        for(std::size_t to = 0; to < grid.size(); ++to) {
            const auto rowDistance = std::max(from / grid.columns(), to / grid.columns())
                - std::min(from / grid.columns(), to / grid.columns());
            const auto columnDistance = std::max(from % grid.columns(), to % grid.columns())
                - std::min(from % grid.columns(), to % grid.columns());
            const bool edge = rowDistance + columnDistance == 1 && !grid.isBlocked(from) && !grid.isBlocked(to);
            REQUIRE(matrix[from][to] == (from == to ? 0.0 : edge ? 1.0 : inf));
        }
    }
    Dijkstra search(matrix, 6, 24);
    REQUIRE(search.run(false).found);
}

TEST_CASE("Random maze generation rejects invalid endpoints without modifying the grid", "[search][grid][maze]") {
    Grid grid(5);
    grid.setBlocked(3);
    const auto before = obstacleMask(grid);
    const auto matrix = grid.matrix();
    REQUIRE_THROWS_AS(grid.generateMaze(grid.size(), 24, 0), std::out_of_range);
    REQUIRE_THROWS_AS(grid.generateMaze(0, grid.size(), 0), std::out_of_range);
    REQUIRE_THROWS_AS(grid.generateMaze(0, std::numeric_limits<std::size_t>::max(), 0), std::out_of_range);
    REQUIRE(obstacleMask(grid) == before);
    REQUIRE(grid.matrix() == matrix);
}

TEST_CASE("Million-cell random maze generation stays compact and connected", "[search][grid][maze][scale]") {
    auto grid = Grid::fromCellCount(1'000'003);
    constexpr std::size_t start = 333'333;
    const auto goal = grid.size() - 1;
    grid.generateMaze(start, goal, 2026);
    REQUIRE_FALSE(grid.isBlocked(start));
    REQUIRE_FALSE(grid.isBlocked(goal));
    const auto reached = reachableCells(grid, start);
    REQUIRE(reached[goal]);
    bool allOpenCellsConnected = true;
    for(std::size_t node = 0; node < grid.size(); ++node)
        if(reached[node] == grid.isBlocked(node)) allOpenCellsConnected = false;
    REQUIRE(allOpenCellsConnected);
}
