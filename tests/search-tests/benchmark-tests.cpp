#include <catch2/catch_test_macros.hpp>

#include <graphReader.hpp>
#include <searchBenchmark.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr double inf = std::numeric_limits<double>::infinity();

    SearchBenchmark::Matrix weightedGraph() {
        return {
            {0,   9,   2,   20,  50},
            {inf, 0,   inf, 1,   inf},
            {inf, 1,   0,   10,  inf},
            {inf, inf, inf, 0,   1},
            {inf, inf, inf, inf, 0}
        };
    }

    void requireMatchesReference(const SearchBenchmark& benchmark,
                                 const SearchBenchmark::Matrix& graph) {
        Dijkstra reference(graph, benchmark.start(), benchmark.goal());
        const auto& expected = reference.run(false);
        constexpr std::array<Heuristics::Type, 3> order{
            Heuristics::Type::None, Heuristics::Type::Euclidean, Heuristics::Type::Manhattan
        };
        REQUIRE(benchmark.hasResults());
        for(std::size_t index = 0; index < order.size(); ++index) {
            CAPTURE(index, benchmark.start(), benchmark.goal());
            const auto& measurement = benchmark.results()[index];
            const auto& result = measurement.result;
            REQUIRE(measurement.heuristic == order[index]);
            REQUIRE(std::isfinite(measurement.milliseconds));
            REQUIRE(measurement.milliseconds >= 0.0);
            REQUIRE(result.steps.empty());
            REQUIRE(result.found == expected.found);
            REQUIRE(result.totalCost == expected.totalCost);
            REQUIRE(result.heuristics.size() == graph.size());
            REQUIRE(result.heuristics[benchmark.goal()] == 0.0);
            REQUIRE(result.exploredNodes > 0);
            REQUIRE(result.exploredNodes <= graph.size());
            for(const auto estimate : result.heuristics) {
                REQUIRE(std::isfinite(estimate));
                REQUIRE(estimate >= 0.0);
                if(order[index] == Heuristics::Type::None)
                    REQUIRE(estimate == 0.0);
            }
            if(result.found) {
                REQUIRE_FALSE(result.path.empty());
                REQUIRE(result.path.front() == benchmark.start());
                REQUIRE(result.path.back() == benchmark.goal());
                double cost = 0.0;
                for(std::size_t edge = 1; edge < result.path.size(); ++edge) {
                    REQUIRE(result.path[edge - 1] < graph.size());
                    REQUIRE(result.path[edge] < graph.size());
                    cost += graph[result.path[edge - 1]][result.path[edge]];
                }
                REQUIRE(cost == expected.totalCost);
            } else {
                REQUIRE(result.path.empty());
            }
        }
        REQUIRE(benchmark.results()[0].result.path == expected.path);
        REQUIRE(benchmark.results()[0].result.exploredNodes == expected.exploredNodes);
    }

    void requireSameMeasurements(const std::array<SearchBenchmark::Measurement, 3>& actual,
                                 const std::array<SearchBenchmark::Measurement, 3>& expected) {
        for(std::size_t index = 0; index < actual.size(); ++index) {
            REQUIRE(actual[index].heuristic == expected[index].heuristic);
            REQUIRE(actual[index].milliseconds == expected[index].milliseconds);
            REQUIRE(actual[index].result.found == expected[index].result.found);
            REQUIRE(actual[index].result.totalCost == expected[index].result.totalCost);
            REQUIRE(actual[index].result.path == expected[index].result.path);
            REQUIRE(actual[index].result.exploredNodes == expected[index].result.exploredNodes);
            REQUIRE(actual[index].result.heuristics == expected[index].result.heuristics);
            REQUIRE(actual[index].result.steps.empty());
        }
    }
}

TEST_CASE("Benchmark requires a loaded graph and an explicit comparison", "[search][benchmark]") {
    SearchBenchmark benchmark;
    REQUIRE_FALSE(benchmark.loaded());
    REQUIRE_FALSE(benchmark.hasResults());
    REQUIRE(benchmark.nodeCount() == 0);
    REQUIRE_FALSE(benchmark.setStart(0));
    REQUIRE_FALSE(benchmark.setGoal(0));
    REQUIRE_THROWS_AS(benchmark.compare(), std::logic_error);
    REQUIRE_THROWS_AS(benchmark.results(), std::logic_error);

    const auto graph = weightedGraph();
    benchmark.load(graph);
    REQUIRE(benchmark.loaded());
    REQUIRE(benchmark.nodeCount() == graph.size());
    REQUIRE(benchmark.start() == 0);
    REQUIRE(benchmark.goal() == graph.size() - 1);
    REQUIRE_FALSE(benchmark.hasResults());
    REQUIRE_THROWS_AS(benchmark.results(), std::logic_error);

    benchmark.compare();
    requireMatchesReference(benchmark, graph);
    REQUIRE(benchmark.results()[0].result.totalCost == 5.0);
    REQUIRE(benchmark.geometryScale() > 0.0);
    REQUIRE(benchmark.results()[1].result.heuristics[0] > 0.0);
    REQUIRE(benchmark.results()[2].result.heuristics[0] > 0.0);
    benchmark.compare();
    requireMatchesReference(benchmark, graph);
}

TEST_CASE("Benchmark loads the defined matrix format and measures fractional routes", "[search][benchmark]") {
    std::istringstream input("|0, 0.5, 2|\n|INF, 0, 0.25|\n|INF, INF, 0|\n");
    const auto graph = GraphReader::read(input);
    SearchBenchmark benchmark;
    benchmark.load(graph);
    benchmark.compare();
    requireMatchesReference(benchmark, graph);
    for(const auto& measurement : benchmark.results()) {
        REQUIRE(measurement.result.totalCost == 0.75);
        REQUIRE(measurement.result.path == std::vector<std::size_t>{0, 1, 2});
    }
}

TEST_CASE("Benchmark handles directed unreachable goals and shared endpoints", "[search][benchmark]") {
    const auto graph = weightedGraph();
    SearchBenchmark benchmark;
    benchmark.load(graph);
    REQUIRE(benchmark.setStart(4));
    REQUIRE(benchmark.setGoal(0));
    benchmark.compare();
    requireMatchesReference(benchmark, graph);
    for(const auto& measurement : benchmark.results())
        REQUIRE_FALSE(measurement.result.found);

    REQUIRE(benchmark.setGoal(4));
    benchmark.compare();
    requireMatchesReference(benchmark, graph);
    for(const auto& measurement : benchmark.results()) {
        REQUIRE(measurement.result.path == std::vector<std::size_t>{4});
        REQUIRE(measurement.result.totalCost == 0.0);
        REQUIRE(measurement.result.exploredNodes == 1);
    }

    const SearchBenchmark::Matrix single{{inf}};
    benchmark.load(single);
    benchmark.compare();
    requireMatchesReference(benchmark, single);
}

TEST_CASE("Benchmark safely reduces geometric estimates for free edges", "[search][benchmark]") {
    const SearchBenchmark::Matrix graph{
        {0,   0,   0,   inf},
        {0,   0,   0,   1},
        {0,   0,   0,   1},
        {inf, inf, inf, 0}
    };
    SearchBenchmark benchmark;
    benchmark.load(graph);
    REQUIRE(benchmark.geometryScale() == 0.0);
    benchmark.compare();
    requireMatchesReference(benchmark, graph);
    for(const auto& measurement : benchmark.results())
        REQUIRE(measurement.result.heuristics == std::vector<double>(graph.size(), 0.0));
}

TEST_CASE("Benchmark owns graph data and invalidates results only for real changes", "[search][benchmark]") {
    auto graph = weightedGraph();
    const auto original = graph;
    SearchBenchmark benchmark;
    benchmark.load(graph);
    graph[0][4] = 0.0;
    benchmark.compare();
    requireMatchesReference(benchmark, original);
    const auto completed = benchmark.results();

    REQUIRE(benchmark.setStart(benchmark.start()));
    REQUIRE(benchmark.setGoal(benchmark.goal()));
    REQUIRE_FALSE(benchmark.setStart(original.size()));
    REQUIRE_FALSE(benchmark.setGoal(original.size()));
    REQUIRE(benchmark.hasResults());
    REQUIRE(benchmark.start() == 0);
    REQUIRE(benchmark.goal() == 4);
    requireSameMeasurements(benchmark.results(), completed);

    REQUIRE(benchmark.setStart(2));
    REQUIRE_FALSE(benchmark.hasResults());
    REQUIRE_THROWS_AS(benchmark.results(), std::logic_error);
    benchmark.compare();
    requireMatchesReference(benchmark, original);
    REQUIRE(benchmark.setGoal(3));
    REQUIRE_FALSE(benchmark.hasResults());
    benchmark.compare();
    requireMatchesReference(benchmark, original);

    benchmark.load(graph);
    REQUIRE_FALSE(benchmark.hasResults());
    REQUIRE(benchmark.start() == 0);
    REQUIRE(benchmark.goal() == 4);
    benchmark.compare();
    requireMatchesReference(benchmark, graph);
    REQUIRE(benchmark.results()[0].result.totalCost == 0.0);
}

TEST_CASE("Invalid benchmark loads preserve completed results and selected endpoints", "[search][benchmark]") {
    SearchBenchmark benchmark;
    const auto graph = weightedGraph();
    benchmark.load(graph);
    REQUIRE(benchmark.setStart(2));
    REQUIRE(benchmark.setGoal(3));
    benchmark.compare();
    const auto completed = benchmark.results();
    const auto scale = benchmark.geometryScale();
    for(const auto& invalid : std::vector<SearchBenchmark::Matrix>{
        {}, {{0, 1}}, {{0, 1}, {0}}, {{0, -1}, {inf, 0}},
        {{std::numeric_limits<double>::quiet_NaN()}}, {{-inf}}
    }) {
        REQUIRE_THROWS_AS(benchmark.load(invalid), std::invalid_argument);
        REQUIRE(benchmark.loaded());
        REQUIRE(benchmark.nodeCount() == graph.size());
        REQUIRE(benchmark.start() == 2);
        REQUIRE(benchmark.goal() == 3);
        REQUIRE(benchmark.geometryScale() == scale);
        REQUIRE(benchmark.hasResults());
        requireSameMeasurements(benchmark.results(), completed);
    }
    benchmark.compare();
    requireMatchesReference(benchmark, graph);
}

TEST_CASE("All benchmark heuristics agree on generated weighted directed graphs", "[search][benchmark]") {
    std::mt19937 random(20261101);
    for(std::size_t count = 2; count <= 9; ++count) {
        SearchBenchmark::Matrix graph(count, std::vector<double>(count, inf));
        for(std::size_t from = 0; from < count; ++from) {
            graph[from][from] = 0;
            for(std::size_t to = 0; to < count; ++to)
                if(from != to && random() % 3 != 0)
                    graph[from][to] = static_cast<double>(1 + random() % 64) / 8.0;
        }
        SearchBenchmark benchmark;
        benchmark.load(graph);
        for(std::size_t start = 0; start < count; ++start) {
            for(std::size_t goal = 0; goal < count; ++goal) {
                CAPTURE(count, start, goal);
                REQUIRE(benchmark.setStart(start));
                REQUIRE(benchmark.setGoal(goal));
                benchmark.compare();
                requireMatchesReference(benchmark, graph);
            }
        }
    }
}
