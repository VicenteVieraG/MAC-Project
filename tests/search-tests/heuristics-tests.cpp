#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <dijkstra.hpp>
#include <grid.hpp>
#include <heuristics.hpp>

#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>

namespace {
    constexpr double inf = std::numeric_limits<double>::infinity();
    using Heuristics::Position;
    using Heuristics::Type;

}

TEST_CASE("Geometric heuristics use distance directly as estimated cost", "[search][heuristics]") {
    const Position node{-2.0, -1.0};
    const Position goal{1.0, 3.0};
    REQUIRE(Heuristics::manhattan(node, goal) == 7.0);
    REQUIRE(Heuristics::manhattan(goal, node) == 7.0);
    REQUIRE(Heuristics::euclidean(node, goal) == 5.0);
    REQUIRE(Heuristics::euclidean(goal, node) == 5.0);
    REQUIRE(Heuristics::euclidean({0, 0}, {1, 1}) == Catch::Approx(std::sqrt(2.0)));
    for(const auto type : {Type::None, Type::Manhattan, Type::Euclidean}) {
        REQUIRE(Heuristics::estimate(type, goal, goal) == 0.0);
    }
    REQUIRE(Heuristics::estimate(Type::None, node, goal) == 0.0);
    REQUIRE(Heuristics::estimate(Type::Manhattan, node, goal) == 7.0);
    REQUIRE(Heuristics::estimate(Type::Euclidean, node, goal) == 5.0);
    // Squaring this finite distance would overflow; hypot must not.
    REQUIRE(Heuristics::euclidean({0, 0}, {1e200, 0}) == 1e200);
}

TEST_CASE("Heuristics reject invalid inputs and nonfinite estimates", "[search][heuristics]") {
    for(const auto invalid : {inf, -inf, std::numeric_limits<double>::quiet_NaN()}) {
        REQUIRE_THROWS_AS(Heuristics::manhattan({invalid, 0}, {0, 0}), std::invalid_argument);
        REQUIRE_THROWS_AS(Heuristics::euclidean({0, 0}, {0, invalid}), std::invalid_argument);
        REQUIRE_THROWS_AS(Heuristics::estimate(Type::None, {0, 0}, {invalid, 0}), std::invalid_argument);
        REQUIRE_THROWS_AS(Heuristics::estimate(Type::Manhattan, {0, invalid}, {1, 0}), std::invalid_argument);
    }
    REQUIRE_THROWS_AS(Heuristics::estimate(static_cast<Type>(99), {0, 0}, {0, 0}), std::invalid_argument);
    const auto max = std::numeric_limits<double>::max();
    REQUIRE_THROWS_AS(Heuristics::manhattan({max, max}, {0, 0}), std::overflow_error);
    REQUIRE_THROWS_AS(Heuristics::euclidean({max, 0}, {-max, 0}), std::overflow_error);
    REQUIRE_THROWS_AS(Heuristics::estimate(Type::Manhattan, {max, max}, {0, 0}), std::overflow_error);
}

TEST_CASE("A star improves frontier priorities while retaining real path costs", "[search][astar]") {
    const Dijkstra::Matrix graph{
        {0,   9,   2,  20,  50},
        {inf, 0,   inf, 1,  inf},
        {inf, 1,   0,  10,  inf},
        {inf, inf, inf, 0,    1},
        {inf, inf, inf, inf,  0}
    };
    for(const auto type : {Type::Manhattan, Type::Euclidean}) {
        auto positions = std::vector<Position>{{0, 0}, {0, 2}, {0, 1}, {0, 3}, {0, 4}};
        Dijkstra search(graph, 0, 4, positions, type);
        positions[4] = {999, 999}; // Construction owns the derived estimates.
        REQUIRE_THROWS_AS(search.result(), std::logic_error);
        const auto recorded = search.run();
        REQUIRE(recorded.found);
        REQUIRE(recorded.totalCost == 5.0);
        REQUIRE(recorded.path == std::vector<Dijkstra::NodeId>{0, 2, 1, 3, 4});
        REQUIRE(recorded.heuristics == std::vector<double>{4, 2, 3, 1, 0});
        REQUIRE(recorded.steps.front().priorities == std::vector<double>{4, inf, inf, inf, inf});
        REQUIRE(recorded.steps[1].distances[1] == 9.0);
        REQUIRE(recorded.steps[1].priorities[1] == 11.0);
        REQUIRE(recorded.steps[2].distances[1] == 3.0);
        REQUIRE(recorded.steps[2].priorities[1] == 5.0);
        REQUIRE(recorded.steps.size() == recorded.exploredNodes + 1);
        for(const auto& step : recorded.steps) {
            for(std::size_t node = 0; node < graph.size(); ++node) {
                REQUIRE(step.priorities[node] == step.distances[node] + recorded.heuristics[node]);
                REQUIRE_FALSE((step.visited[node] && step.inFrontier[node]));
            }
        }
        const auto& rerun = search.run(false);
        REQUIRE(&rerun == &search.result());
        REQUIRE(rerun.steps.empty());
        REQUIRE(rerun.path == recorded.path);
        REQUIRE(rerun.totalCost == recorded.totalCost);
        REQUIRE(rerun.exploredNodes == recorded.exploredNodes);
        REQUIRE(rerun.heuristics == recorded.heuristics);
    }
}

TEST_CASE("Both heuristics guide search toward the goal", "[search][astar]") {
    const Grid grid(5);
    Dijkstra baseline(grid.matrix(), 0, 4);
    const auto expected = baseline.run();
    for(const auto type : {Type::Manhattan, Type::Euclidean}) {
        Dijkstra search(grid.matrix(), 0, 4, grid.positions(), type);
        const auto& result = search.run();
        REQUIRE(result.totalCost == expected.totalCost);
        REQUIRE(result.path == expected.path);
        REQUIRE(result.exploredNodes == 5);
        REQUIRE(result.exploredNodes < expected.exploredNodes);
    }
    Dijkstra disabled(grid.matrix(), 0, 4, grid.positions(), Type::None);
    REQUIRE(disabled.run().exploredNodes == expected.exploredNodes);
    REQUIRE(disabled.result().path == expected.path);
    REQUIRE(disabled.result().heuristics == std::vector<double>(25, 0.0));
    for(const auto& step : disabled.result().steps)
        REQUIRE(step.priorities == step.distances);
}

TEST_CASE("A star handles obstacle detours unreachable goals and identical endpoints", "[search][astar]") {
    Grid detour(5);
    for(const auto node : {2, 7, 12, 17}) detour.setBlocked(node);
    Grid disconnected = detour;
    disconnected.setBlocked(22);
    for(const auto type : {Type::Manhattan, Type::Euclidean}) {
        Dijkstra search(detour.matrix(), 0, 4, detour.positions(), type);
        REQUIRE(search.run().totalCost == 12.0);
        REQUIRE(search.result().path.size() == 13);
        Dijkstra unreachable(disconnected.matrix(), 0, 4, disconnected.positions(), type);
        const auto& result = unreachable.run();
        REQUIRE_FALSE(result.found);
        REQUIRE(result.totalCost == inf);
        REQUIRE(result.path.empty());
        REQUIRE(result.steps.back().inFrontier == std::vector<bool>(25, false));
        Dijkstra same(detour.matrix(), 3, 3, detour.positions(), type);
        REQUIRE(same.run().totalCost == 0.0);
        REQUIRE(same.result().path == std::vector<Dijkstra::NodeId>{3});
        REQUIRE(same.result().exploredNodes == 1);
    }
}

TEST_CASE("A star validates coordinates and consistency against directed edge costs", "[search][astar]") {
    const Dijkstra::Matrix graph{{0, 0.5}, {inf, 0}};
    const std::vector<Position> positions{{0, 0}, {0, 1}};
    REQUIRE_THROWS_AS((Dijkstra(graph, 0, 1, {}, Type::Manhattan)), std::invalid_argument);
    REQUIRE_THROWS_AS((Dijkstra(graph, 0, 1, {{0, 0}, {inf, 1}}, Type::Euclidean)), std::invalid_argument);
    // Generic graph input can still contain weights incompatible with the estimates.
    for(const auto type : {Type::Manhattan, Type::Euclidean}) {
        REQUIRE_THROWS_AS((Dijkstra(graph, 0, 1, positions, type)), std::invalid_argument);
    }
    Dijkstra disabled(graph, 0, 1, positions, Type::None);
    REQUIRE(disabled.run().totalCost == 0.5);
    Dijkstra zero({{0, 0}, {0, 0}}, 1, 0, positions, Type::None);
    REQUIRE(zero.run().totalCost == 0.0);
    REQUIRE(zero.result().path == std::vector<Dijkstra::NodeId>{1, 0});
    // A sqrt(2)-cost diagonal permits Euclidean, but not raw Manhattan.
    const Dijkstra::Matrix diagonal{{0, std::sqrt(2.0)}, {inf, 0}};
    const std::vector<Position> diagonalPositions{{0, 0}, {1, 1}};
    REQUIRE_THROWS_AS((Dijkstra(diagonal, 0, 1, diagonalPositions, Type::Manhattan)), std::invalid_argument);
    Dijkstra euclidean(diagonal, 0, 1, diagonalPositions, Type::Euclidean);
    REQUIRE(euclidean.run().totalCost == std::sqrt(2.0));
}

TEST_CASE("A star detects priority overflow independently of path cost", "[search][astar]") {
    const auto max = std::numeric_limits<double>::max();
    Dijkstra search({{0, max, inf}, {inf, 0, max}, {inf, inf, 0}}, 0, 2,
                    {{0, 0}, {0, 0}, {0, max / 2}}, Type::Manhattan);
    REQUIRE_THROWS_AS(search.run(), std::overflow_error);
    REQUIRE_THROWS_AS(search.result(), std::logic_error);
}

TEST_CASE("A star matches Dijkstra on generated unit cost obstacle maps", "[search][astar]") {
    std::mt19937 random(20261031);
    constexpr std::size_t side = 7;
    constexpr std::size_t count = side * side;
    for(std::size_t sample = 0; sample < 12; ++sample) {
        Grid grid(side);
        for(std::size_t node = 0; node < count; ++node)
            if(random() % 4 == 0) grid.setBlocked(node);

        for(std::size_t trial = 0; trial < 8; ++trial) {
            const auto start = random() % count;
            const auto goal = random() % count;
            Dijkstra baseline(grid.matrix(), start, goal);
            const auto expected = baseline.run(false);
            for(const auto type : {Type::Manhattan, Type::Euclidean}) {
                CAPTURE(sample, trial, start, goal);
                Dijkstra search(grid.matrix(), start, goal, grid.positions(), type);
                const auto& result = search.run(false);
                REQUIRE(result.found == expected.found);
                REQUIRE(result.totalCost == expected.totalCost);
                if(!result.found) {
                    REQUIRE(result.path.empty());
                    continue;
                }
                REQUIRE(result.path.front() == start);
                REQUIRE(result.path.back() == goal);
                REQUIRE(result.path.size() <= count);
                double pathCost = 0;
                for(std::size_t index = 1; index < result.path.size(); ++index)
                    pathCost += grid.matrix()[result.path[index - 1]][result.path[index]];
                REQUIRE(pathCost == result.totalCost);
                REQUIRE(result.totalCost == static_cast<double>(result.path.size() - 1));
            }
        }
    }
}
