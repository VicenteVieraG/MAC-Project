#include <catch2/catch_test_macros.hpp>

#include <dijkstra.hpp>
#include <graphReader.hpp>

#include <algorithm>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr double inf = std::numeric_limits<double>::infinity();

    Dijkstra::Matrix exampleGraph() {
        return {
            {0,   9,   2,  20,  50},
            {inf, 0,   inf, 1,  inf},
            {inf, 1,   0,  10,  inf},
            {inf, inf, inf, 0,    1},
            {inf, inf, inf, inf,  0}
        };
    }
}

TEST_CASE("Dijkstra improves queued costs and stops when the goal is popped", "[search][dijkstra]") {
    Dijkstra search(exampleGraph(), 0, 3);
    REQUIRE_THROWS_AS(search.result(), std::logic_error);
    const auto& result = search.run();
    REQUIRE(&result == &search.result());
    REQUIRE(result.found);
    REQUIRE(result.totalCost == 4.0);
    REQUIRE(result.path == std::vector<Dijkstra::NodeId>{0, 2, 1, 3});
    REQUIRE(result.exploredNodes == 4);
    REQUIRE(result.steps.size() == 5);

    const auto& initial = result.steps.front();
    REQUIRE(initial.current == 0);
    REQUIRE(initial.distances == std::vector<double>{0, inf, inf, inf, inf});
    REQUIRE(initial.inFrontier == std::vector<bool>{true, false, false, false, false});
    REQUIRE(initial.visited == std::vector<bool>(5, false));
    REQUIRE(initial.predecessors == std::vector<Dijkstra::NodeId>(5, 0));

    // Earlier snapshots retain tentative costs and predecessors after later updates.
    REQUIRE(result.steps[1].current == 0);
    REQUIRE(result.steps[1].distances[1] == 9.0);
    REQUIRE(result.steps[1].predecessors[1] == 0);
    REQUIRE(result.steps[2].current == 2);
    REQUIRE(result.steps[2].distances[1] == 3.0);
    REQUIRE(result.steps[2].predecessors[1] == 2);
    REQUIRE(result.steps[3].current == 1);
    REQUIRE(result.steps[3].distances[3] == 4.0);

    const auto& final = result.steps.back();
    REQUIRE(final.current == 3);
    REQUIRE(final.visited == std::vector<bool>{true, true, true, true, false});
    REQUIRE(final.inFrontier == std::vector<bool>{false, false, false, false, true});
    REQUIRE(final.distances[4] == 50.0); // Goal's outgoing edges are not expanded.

    for(std::size_t index = 1; index < result.steps.size(); ++index) {
        const auto& step = result.steps[index];
        REQUIRE(std::count(step.visited.begin(), step.visited.end(), true) == index);
        for(std::size_t node = 0; node < step.visited.size(); ++node)
            REQUIRE_FALSE((step.visited[node] && step.inFrontier[node]));
    }
}

TEST_CASE("Dijkstra reports unreachable goals in directed graphs", "[search][dijkstra]") {
    Dijkstra search({{0, 1, inf}, {inf, 0, inf}, {inf, 1, 0}}, 0, 2);
    const auto& result = search.run();
    REQUIRE_FALSE(result.found);
    REQUIRE(result.totalCost == inf);
    REQUIRE(result.path.empty());
    REQUIRE(result.exploredNodes == 2);
    REQUIRE(result.steps.size() == 3);
    REQUIRE(result.steps.back().inFrontier == std::vector<bool>(3, false));
    REQUIRE(result.steps.back().predecessors[2] == 0);
    REQUIRE(result.steps.back().distances[2] == inf);
}

TEST_CASE("Dijkstra handles identical endpoints and absent self edges", "[search][dijkstra]") {
    Dijkstra search({{inf}}, 0, 0);
    const auto& result = search.run();
    REQUIRE(result.found);
    REQUIRE(result.totalCost == 0.0);
    REQUIRE(result.path == std::vector<Dijkstra::NodeId>{0});
    REQUIRE(result.exploredNodes == 1);
    REQUIRE(result.steps.size() == 2);
    REQUIRE(result.steps.back().visited[0]);
    REQUIRE_FALSE(result.steps.back().inFrontier[0]);
}

TEST_CASE("Dijkstra supports zero cost cycles and deterministic equal cost paths", "[search][dijkstra]") {
    Dijkstra search({{0, 0, 0, inf}, {0, 0, 0, 1}, {0, 0, 0, 1}, {inf, inf, inf, 0}}, 0, 3);
    const auto& result = search.run();
    REQUIRE(result.totalCost == 1.0);
    REQUIRE(result.path == std::vector<Dijkstra::NodeId>{0, 1, 3});
    REQUIRE(result.exploredNodes == 4);
    REQUIRE(result.steps[2].current == 1);
    REQUIRE(result.steps[3].current == 2);
    REQUIRE(result.steps.back().predecessors[0] == 0);
}

TEST_CASE("Dijkstra reconstructs paths through node zero from a nonzero start", "[search][dijkstra]") {
    Dijkstra search({{0, 1, inf}, {inf, 0, inf}, {2, inf, 0}}, 2, 1);
    const auto& result = search.run();
    REQUIRE(result.found);
    REQUIRE(result.totalCost == 3.0);
    REQUIRE(result.path == std::vector<Dijkstra::NodeId>{2, 0, 1});
    REQUIRE(result.steps.front().current == 2);
    REQUIRE(result.steps.front().predecessors == std::vector<Dijkstra::NodeId>(3, 0));
    REQUIRE(result.steps.back().predecessors[0] == 2);
    REQUIRE(result.steps.back().predecessors[1] == 0);
}

TEST_CASE("Dijkstra owns its input and keeps instances and repeated runs independent", "[search][dijkstra]") {
    auto graph = exampleGraph();
    Dijkstra first(graph, 0, 3);
    Dijkstra second(graph, 3, 0);
    graph[0][3] = 0.0;
    const auto recorded = first.run();
    REQUIRE(recorded.totalCost == 4.0);
    REQUIRE_FALSE(second.run().found);
    REQUIRE(first.result().found);

    const auto& unrecorded = first.run(false);
    REQUIRE(unrecorded.steps.empty());
    REQUIRE(unrecorded.found == recorded.found);
    REQUIRE(unrecorded.totalCost == recorded.totalCost);
    REQUIRE(unrecorded.path == recorded.path);
    REQUIRE(unrecorded.exploredNodes == recorded.exploredNodes);
    REQUIRE(first.run().steps.size() == recorded.steps.size());
    REQUIRE_FALSE(second.result().found);
    REQUIRE(recorded.steps[1].distances[3] == 20.0);
}

TEST_CASE("Dijkstra validates the entire input before searching", "[search][dijkstra]") {
    REQUIRE_THROWS_AS((Dijkstra({}, 0, 0)), std::invalid_argument);
    REQUIRE_THROWS_AS((Dijkstra({{0, 1}, {0}}, 0, 1)), std::invalid_argument);
    REQUIRE_THROWS_AS((Dijkstra({{0, 1}}, 0, 0)), std::invalid_argument);
    REQUIRE_THROWS_AS((Dijkstra({{0}}, 1, 0)), std::out_of_range);
    REQUIRE_THROWS_AS((Dijkstra({{0}}, 0, 1)), std::out_of_range);
    for(const auto invalid : {-1.0, -inf, std::numeric_limits<double>::quiet_NaN()}) {
        // Even an unreachable edge must be validated, including when start == goal.
        REQUIRE_THROWS_AS((Dijkstra({{0, inf}, {inf, invalid}}, 0, 0)), std::invalid_argument);
    }
}

TEST_CASE("Dijkstra detects path cost overflow instead of reporting unreachable", "[search][dijkstra]") {
    const auto max = std::numeric_limits<double>::max();
    Dijkstra search({{0, max, inf}, {inf, 0, max}, {inf, inf, 0}}, 0, 2);
    REQUIRE_THROWS_AS(search.run(), std::overflow_error);
    REQUIRE_THROWS_AS(search.result(), std::logic_error);
    Dijkstra finite({{0, max}, {inf, 0}}, 0, 1);
    REQUIRE(finite.run().totalCost == max);
}

TEST_CASE("Dijkstra accepts matrices returned by GraphReader", "[search][dijkstra]") {
    std::istringstream input("|0, 0.5, 2|\n|INF, 0, 0.25|\n|INF, INF, 0|\n");
    Dijkstra search(GraphReader::read(input), 0, 2);
    const auto& result = search.run();
    REQUIRE(result.totalCost == 0.75);
    REQUIRE(result.path == std::vector<Dijkstra::NodeId>{0, 1, 2});
}

TEST_CASE("Dijkstra matches all pairs reference costs on generated graphs", "[search][dijkstra]") {
    std::mt19937 random(20261030);
    constexpr std::size_t count = 7;
    for(std::size_t sample = 0; sample < 12; ++sample) {
        Dijkstra::Matrix graph(count, std::vector<double>(count, inf));
        for(std::size_t from = 0; from < count; ++from) {
            graph[from][from] = 0;
            for(std::size_t to = 0; to < count; ++to)
                if(from != to && random() % 3 == 0)
                    graph[from][to] = static_cast<double>(random() % 10);
        }

        // Floyd-Warshall supplies an independent reference without a frontier heap.
        auto expected = graph;
        for(std::size_t via = 0; via < count; ++via)
            for(std::size_t from = 0; from < count; ++from)
                for(std::size_t to = 0; to < count; ++to)
                    expected[from][to] = std::min(expected[from][to], expected[from][via] + expected[via][to]);

        for(std::size_t start = 0; start < count; ++start) {
            for(std::size_t goal = 0; goal < count; ++goal) {
                CAPTURE(sample, start, goal);
                Dijkstra search(graph, start, goal);
                const auto& result = search.run(false);
                REQUIRE(result.totalCost == expected[start][goal]);
                REQUIRE(result.found == (expected[start][goal] != inf));
                if(!result.found) {
                    REQUIRE(result.path.empty());
                    continue;
                }
                REQUIRE_FALSE(result.path.empty());
                REQUIRE(result.path.front() == start);
                REQUIRE(result.path.back() == goal);
                REQUIRE(result.path.size() <= count);
                double pathCost = 0;
                for(std::size_t index = 1; index < result.path.size(); ++index)
                    pathCost += graph[result.path[index - 1]][result.path[index]];
                REQUIRE(pathCost == result.totalCost);
            }
        }
    }
}
