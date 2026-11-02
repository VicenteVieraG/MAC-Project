#include <catch2/catch_test_macros.hpp>

#include <graphPlayback.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
    constexpr double inf = std::numeric_limits<double>::infinity();

    GraphPlayback::Matrix weightedGraph() {
        // Directed edges include a free move and successive cost improvements.
        return {
            {0,   9,   2,   20,  50},
            {inf, 0,   inf, 0,   inf},
            {inf, 1,   0,   10,  inf},
            {inf, inf, inf, 0,   1},
            {inf, inf, inf, inf, 0}
        };
    }

    void requireState(const Dijkstra::SearchStep& actual, const Dijkstra::SearchStep& expected) {
        REQUIRE(actual.current == expected.current);
        REQUIRE(actual.distances == expected.distances);
        REQUIRE(actual.predecessors == expected.predecessors);
        REQUIRE(actual.visited == expected.visited);
        REQUIRE(actual.inFrontier == expected.inFrontier);
        REQUIRE(actual.priorities == expected.priorities);
    }
}

TEST_CASE("Graph playback is inert before a graph is loaded", "[search][graph][graph-playback]") {
    GraphPlayback playback;
    REQUIRE_FALSE(playback.loaded());
    REQUIRE(playback.graph().empty());
    REQUIRE(playback.step() == 0);
    REQUIRE(playback.totalSteps() == 0);
    REQUIRE_FALSE(playback.finished());
    REQUIRE_FALSE(playback.found());
    playback.play();
    playback.update(1.0);
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE_FALSE(playback.next());
    REQUIRE_FALSE(playback.previous());
    REQUIRE_FALSE(playback.setStart(0));
    REQUIRE_FALSE(playback.setGoal(0));
    REQUIRE_THROWS_AS(playback.state(), std::logic_error);
    REQUIRE_THROWS_AS(playback.result(), std::logic_error);
}

TEST_CASE("Graph playback restores Dijkstra state when stepping backward and replaying", "[search][graph][graph-playback]") {
    GraphPlayback playback;
    const auto matrix = weightedGraph();
    Dijkstra reference(matrix, 0, 4);
    const auto expected = reference.run();
    playback.load(matrix);
    REQUIRE(playback.loaded());
    REQUIRE(playback.start() == 0);
    REQUIRE(playback.goal() == 4);
    REQUIRE(playback.graph() == matrix);
    REQUIRE(playback.totalSteps() == expected.exploredNodes);
    REQUIRE(playback.result().heuristics == std::vector<double>(5, 0.0));
    requireState(playback.state(), expected.steps.front());
    for(std::size_t step = 1; step < expected.steps.size(); ++step) {
        REQUIRE_FALSE(playback.found());
        REQUIRE(playback.next());
        REQUIRE(playback.step() == step);
        requireState(playback.state(), expected.steps[step]);
    }
    REQUIRE(playback.finished());
    REQUIRE(playback.found());
    REQUIRE(playback.result().totalCost == 4.0);
    REQUIRE(playback.result().path == std::vector<std::size_t>{0, 2, 1, 3, 4});
    REQUIRE_FALSE(playback.next());
    playback.play();
    REQUIRE_FALSE(playback.isPlaying());
    for(std::size_t step = expected.steps.size() - 1; step > 0; --step) {
        REQUIRE(playback.previous());
        REQUIRE_FALSE(playback.finished());
        REQUIRE_FALSE(playback.found());
        requireState(playback.state(), expected.steps[step - 1]);
    }
    REQUIRE_FALSE(playback.previous());
    for(std::size_t step = 1; step < expected.steps.size(); ++step) {
        REQUIRE(playback.next());
        requireState(playback.state(), expected.steps[step]);
    }
    REQUIRE(playback.found());
}

TEST_CASE("Failed graph loads preserve the active graph and playback", "[search][graph][graph-playback]") {
    GraphPlayback playback;
    playback.load(weightedGraph());
    playback.setSpeed(17.0f);
    playback.next();
    playback.play();
    const auto before = playback.state();
    const auto elapsed = playback.elapsedMilliseconds();
    for(const auto& invalid : std::vector<GraphPlayback::Matrix>{
        {}, {{0, 1}}, {{0, -1}, {inf, 0}}, {{std::numeric_limits<double>::quiet_NaN()}},
        {{-inf}}, GraphPlayback::Matrix(GraphPlayback::maxNodes + 1)
    }) {
        REQUIRE_THROWS_AS(playback.load(invalid), std::invalid_argument);
        REQUIRE(playback.graph() == weightedGraph());
        REQUIRE(playback.isPlaying());
        REQUIRE(playback.step() == 1);
        REQUIRE(playback.speed() == 17.0f);
        REQUIRE(playback.start() == 0);
        REQUIRE(playback.goal() == 4);
        REQUIRE(playback.elapsedMilliseconds() == elapsed);
        requireState(playback.state(), before);
    }
    REQUIRE_THROWS_AS(playback.load(weightedGraph(), 5, 4), std::out_of_range);
    const auto huge = std::numeric_limits<double>::max();
    REQUIRE_THROWS_AS(playback.load({{0, huge, inf}, {inf, 0, huge}, {inf, inf, 0}}), std::overflow_error);
    REQUIRE(playback.graph() == weightedGraph());
    REQUIRE(playback.isPlaying());
    requireState(playback.state(), before);
}

TEST_CASE("Graph endpoint edits reset playback and permit a shared start and goal", "[search][graph][graph-playback]") {
    GraphPlayback playback;
    playback.load(weightedGraph());
    playback.play();
    REQUIRE_FALSE(playback.setStart(2));
    REQUIRE_FALSE(playback.setGoal(3));
    playback.pause();
    REQUIRE(playback.next());
    REQUIRE(playback.setStart(2));
    REQUIRE(playback.step() == 0);
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.start() == 2);
    REQUIRE(playback.goal() == 4);
    REQUIRE(playback.state().distances[2] == 0.0);
    REQUIRE(playback.state().distances[0] == inf);
    REQUIRE_FALSE(playback.setStart(2));
    REQUIRE_THROWS_AS(playback.setGoal(5), std::out_of_range);
    REQUIRE(playback.goal() == 4);
    REQUIRE(playback.setGoal(2));
    REQUIRE(playback.totalSteps() == 1);
    REQUIRE_FALSE(playback.found());
    REQUIRE(playback.next());
    REQUIRE(playback.found());
    REQUIRE(playback.result().totalCost == 0.0);
    REQUIRE(playback.result().path == std::vector<std::size_t>{2});

    playback.load({{inf}});
    REQUIRE(playback.start() == playback.goal());
    REQUIRE(playback.next());
    REQUIRE(playback.found());
}

TEST_CASE("Graph playback reports unreachable directed goals and rewinds completion", "[search][graph][graph-playback]") {
    GraphPlayback playback;
    playback.load(weightedGraph(), 4, 0);
    playback.setSpeed(4.0f);
    playback.play();
    playback.update(0.25);
    REQUIRE(playback.finished());
    REQUIRE_FALSE(playback.found());
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.totalSteps() == 1);
    REQUIRE(playback.result().path.empty());
    REQUIRE(playback.result().totalCost == inf);
    REQUIRE(playback.previous());
    REQUIRE_FALSE(playback.finished());
    REQUIRE(playback.next());
    REQUIRE(playback.finished());
}

TEST_CASE("Graph playback timing accumulates fractional steps and pause clears them", "[search][graph][graph-playback]") {
    GraphPlayback playback;
    playback.load(weightedGraph());
    REQUIRE(std::isfinite(playback.elapsedMilliseconds()));
    REQUIRE(playback.elapsedMilliseconds() >= 0.0);
    const auto elapsed = playback.elapsedMilliseconds();
    playback.setSpeed(4.0f);
    playback.play();
    playback.update(0.125);
    REQUIRE(playback.step() == 0);
    playback.update(0.125);
    REQUIRE(playback.step() == 1);
    playback.update(0.125);
    playback.pause();
    playback.update(0.25);
    playback.play();
    playback.update(0.125);
    REQUIRE(playback.step() == 1);
    playback.update(-1.0);
    playback.update(inf);
    playback.update(std::numeric_limits<double>::quiet_NaN());
    REQUIRE(playback.step() == 1);
    REQUIRE(playback.next());
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.step() == 2);
    playback.restart();
    REQUIRE(playback.step() == 0);
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.speed() == 4.0f);
    REQUIRE(playback.start() == 0);
    REQUIRE(playback.goal() == 4);
    REQUIRE(playback.graph() == weightedGraph());
    REQUIRE(playback.elapsedMilliseconds() == elapsed);
    playback.play();
    playback.update(100.0); // Stalled frames catch up by at most 0.25 seconds.
    REQUIRE(playback.step() == 1);
    playback.setSpeed(-10.0f);
    REQUIRE(playback.speed() == GraphPlayback::minSpeed);
    playback.setSpeed(1e8f);
    REQUIRE(playback.speed() == GraphPlayback::maxSpeed);
    REQUIRE_THROWS_AS(playback.setSpeed(std::numeric_limits<float>::infinity()), std::invalid_argument);
    REQUIRE_THROWS_AS(playback.setSpeed(std::numeric_limits<float>::quiet_NaN()), std::invalid_argument);
    REQUIRE(playback.speed() == GraphPlayback::maxSpeed);
    playback.update(0.25);
    REQUIRE(playback.finished());
    REQUIRE_FALSE(playback.isPlaying());
}

TEST_CASE("Graph playback supports its maximum node count", "[search][graph][graph-playback]") {
    GraphPlayback::Matrix graph(GraphPlayback::maxNodes, std::vector<double>(GraphPlayback::maxNodes, inf));
    for(std::size_t node = 1; node < graph.size(); ++node) graph[node - 1][node] = 1;
    GraphPlayback playback;
    playback.load(std::move(graph));
    playback.setSpeed(GraphPlayback::maxSpeed);
    playback.play();
    playback.update(60.0);
    REQUIRE(playback.step() == 250);
    while(playback.isPlaying()) playback.update(0.25);
    REQUIRE(playback.found());
    REQUIRE(playback.result().path.size() == GraphPlayback::maxNodes);
    REQUIRE(playback.result().totalCost == GraphPlayback::maxNodes - 1);
}
