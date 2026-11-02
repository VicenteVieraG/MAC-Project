#include <catch2/catch_test_macros.hpp>

#include <dijkstra.hpp>
#include <gridPlayback.hpp>

#include <cmath>
#include <random>
#include <stdexcept>

namespace {
    void requireSnapshot(const GridPlayback& playback, const Dijkstra::SearchStep& expected) {
        REQUIRE(playback.current() == expected.current);
        for(std::size_t node = 0; node < playback.grid().size(); ++node) {
            REQUIRE(playback.distance(node) == expected.distances[node]);
            REQUIRE((playback.state(node) == GridPlayback::CellState::Visited) == expected.visited[node]);
            REQUIRE((playback.state(node) == GridPlayback::CellState::Frontier) == expected.inFrontier[node]);
        }
    }
}

TEST_CASE("Grid playback matches Dijkstra snapshots in both directions", "[search][grid][playback]") {
    for(const auto type : {Heuristics::Type::None, Heuristics::Type::Manhattan, Heuristics::Type::Euclidean}) {
        GridPlayback playback(36);
        playback.setHeuristic(type);
        // A wall forces a detour; the goal is above the end of the wall.
        playback.setGoal(5);
        for(const auto node : {2, 8, 14, 20, 26}) playback.setBlocked(node, true);
        Dijkstra reference(playback.grid().matrix(), playback.start(), playback.goal(), playback.grid().positions(), type);
        const auto& expected = reference.run();
        REQUIRE(expected.found);
        requireSnapshot(playback, expected.steps.front());
        for(std::size_t step = 1; step < expected.steps.size(); ++step) {
            REQUIRE(playback.next());
            REQUIRE(playback.step() == step);
            requireSnapshot(playback, expected.steps[step]);
        }
        REQUIRE(playback.found());
        REQUIRE(playback.path() == expected.path);
        REQUIRE_FALSE(playback.next());
        for(auto step = expected.steps.size() - 1; step > 0; --step) {
            REQUIRE(playback.previous());
            REQUIRE_FALSE(playback.finished());
            REQUIRE(playback.path().empty());
            requireSnapshot(playback, expected.steps[step - 1]);
        }
        REQUIRE_FALSE(playback.previous());
        // Replay recorded steps, including restoring the terminal path.
        for(std::size_t step = 1; step < expected.steps.size(); ++step) {
            REQUIRE(playback.next());
            requireSnapshot(playback, expected.steps[step]);
        }
        REQUIRE(playback.path() == expected.path);
    }
}

TEST_CASE("Grid playback can resume generating steps after rewinding an incomplete search", "[search][grid][playback]") {
    std::mt19937 random(4231);
    for(const auto type : {Heuristics::Type::None, Heuristics::Type::Manhattan, Heuristics::Type::Euclidean}) {
        for(int map = 0; map < 8; ++map) {
            GridPlayback playback(100);
            playback.setHeuristic(type);
            for(std::size_t node = 1; node < 99; ++node)
                if(random() % 5 == 0) playback.setBlocked(node, true);
            Dijkstra reference(playback.grid().matrix(), playback.start(), playback.goal(), playback.grid().positions(), type);
            const auto& expected = reference.run();
            for(int step = 0; step < 7 && !playback.finished(); ++step) playback.next();
            while(playback.previous()) {}
            for(std::size_t step = 1; step < expected.steps.size(); ++step) {
                REQUIRE(playback.next());
                requireSnapshot(playback, expected.steps[step]);
            }
            REQUIRE(playback.finished());
            REQUIRE(playback.found() == expected.found);
            REQUIRE(playback.path() == expected.path);
        }
    }
}

TEST_CASE("Grid playback locks every edit while playing and resets after a paused edit", "[search][grid][playback]") {
    GridPlayback playback(30);
    playback.setBlocked(7, true);
    playback.play();
    const auto revision = playback.revision();
    REQUIRE_FALSE(playback.resize(100));
    REQUIRE_FALSE(playback.setBlocked(8, true));
    REQUIRE_FALSE(playback.setStart(3));
    REQUIRE_FALSE(playback.setGoal(4));
    REQUIRE_FALSE(playback.setHeuristic(Heuristics::Type::None));
    REQUIRE_FALSE(playback.clearObstacles());
    REQUIRE_FALSE(playback.generateMaze(1234));
    REQUIRE_FALSE(playback.generateMaze());
    REQUIRE(playback.revision() == revision);
    REQUIRE(playback.grid().isBlocked(7));
    playback.setSpeed(40.0f);
    playback.update(0.125);
    REQUIRE(playback.step() == 5);
    playback.pause();
    playback.update(0.25);
    REQUIRE(playback.step() == 5);
    REQUIRE(playback.previous());
    REQUIRE(playback.setBlocked(8, true));
    REQUIRE(playback.step() == 0);
    REQUIRE_FALSE(playback.previous());
    REQUIRE(playback.setStart(7)); // Moving an endpoint clears its obstacle.
    REQUIRE_FALSE(playback.grid().isBlocked(7));
    REQUIRE_FALSE(playback.setBlocked(playback.start(), true));
    REQUIRE_FALSE(playback.setBlocked(playback.goal(), true));
    REQUIRE_FALSE(playback.setGoal(playback.start()));
    REQUIRE_FALSE(playback.setStart(playback.goal()));
    playback.play();
    playback.restart();
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.step() == 0);
    REQUIRE(playback.start() == 7);
    REQUIRE(playback.goal() == 29);
    REQUIRE(playback.grid().isBlocked(8));
    REQUIRE(playback.speed() == 40.0f);
    REQUIRE(playback.heuristic() == Heuristics::Type::Manhattan);
    REQUIRE(playback.clearObstacles());
    REQUIRE_FALSE(playback.grid().isBlocked(8));
}

TEST_CASE("Unreachable grid playback finishes and can be rewound", "[search][grid][playback]") {
    GridPlayback playback(30);
    playback.setBlocked(1, true);
    playback.setBlocked(6, true);
    playback.play();
    playback.update(0.1);
    REQUIRE(playback.finished());
    REQUIRE_FALSE(playback.found());
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.step() == 1);
    playback.play();
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.previous());
    REQUIRE_FALSE(playback.finished());
    REQUIRE(playback.next());
    REQUIRE(playback.finished());
}

TEST_CASE("Grid playback speed accumulates fractional steps and bounds per-frame work", "[search][grid][playback]") {
    GridPlayback playback;
    playback.setSpeed(4.0f);
    playback.play();
    playback.update(0.125);
    REQUIRE(playback.step() == 0);
    playback.update(0.125);
    REQUIRE(playback.step() == 1);
    playback.update(-1.0);
    playback.update(std::numeric_limits<double>::quiet_NaN());
    REQUIRE(playback.step() == 1);
    playback.update(0.125);
    playback.pause();
    playback.play();
    playback.update(0.125); // Pause discarded the previous half-step.
    REQUIRE(playback.step() == 1);
    REQUIRE(playback.next());
    REQUIRE_FALSE(playback.isPlaying());
    REQUIRE(playback.step() == 2);
    playback.setSpeed(0);
    REQUIRE(playback.speed() == GridPlayback::minSpeed);
    playback.setSpeed(1e8f);
    REQUIRE(playback.speed() == GridPlayback::maxSpeed);
    REQUIRE_THROWS_AS(playback.setSpeed(std::numeric_limits<float>::infinity()), std::invalid_argument);
}

TEST_CASE("Grid sizes preserve exact cell counts and partial-row boundaries", "[search][grid][playback]") {
    REQUIRE_THROWS_AS(GridPlayback(29), std::out_of_range);
    REQUIRE(GridPlayback::maxCells == 10'000'000);
    REQUIRE_THROWS_AS(GridPlayback(static_cast<std::size_t>(GridPlayback::maxCells) + 1), std::out_of_range);
    REQUIRE_THROWS_AS(Grid::fromCellCount(0), std::invalid_argument);
    GridPlayback playback(31);
    REQUIRE(playback.grid().size() == 31);
    REQUIRE(playback.grid().columns() == 6);
    REQUIRE(playback.grid().rows() == 6);
    REQUIRE(playback.grid().nodeId(5, 0) == 30);
    REQUIRE_THROWS_AS(playback.grid().nodeId(5, 1), std::out_of_range);
    REQUIRE(playback.grid().neighbors(30) == std::array<std::size_t, 4>{24, 31, 31, 31});
    REQUIRE(playback.grid().neighbors(29) == std::array<std::size_t, 4>{23, 28, 31, 31});
    REQUIRE_THROWS_AS(playback.resize(0), std::out_of_range);
    REQUIRE(playback.grid().size() == 31);
    REQUIRE(playback.resize(30));
    REQUIRE(playback.grid().size() == 30);
    REQUIRE(playback.goal() == 29);
}

TEST_CASE("Maze generation clears completed or rewound search history while preserving setup", "[search][grid][playback][maze]") {
    bool rewind = false;
    SECTION("Regenerate after completion") {}
    SECTION("Regenerate after rewinding a completed search") { rewind = true; }
    for(const auto type : {Heuristics::Type::None, Heuristics::Type::Manhattan, Heuristics::Type::Euclidean}) {
        // Both endpoints lie between maze rooms; the goal is in the partial last row.
        GridPlayback playback(47);
        REQUIRE(playback.setStart(8));
        REQUIRE(playback.setGoal(43));
        playback.setHeuristic(type);
        playback.setSpeed(125.0f);
        playback.setBlocked(14, true);
        for(std::size_t step = 0; step < playback.grid().size() && !playback.finished(); ++step)
            REQUIRE(playback.next());
        REQUIRE(playback.found());
        REQUIRE_FALSE(playback.path().empty());

        if(rewind) {
            REQUIRE(playback.previous());
            REQUIRE(playback.previous());
            REQUIRE_FALSE(playback.finished());
        }

        const auto revision = playback.revision();
        REQUIRE(playback.generateMaze(984));
        REQUIRE(playback.grid().size() == 47);
        REQUIRE(playback.grid().columns() == 7);
        REQUIRE(playback.grid().rows() == 7);
        REQUIRE(playback.start() == 8);
        REQUIRE(playback.goal() == 43);
        REQUIRE(playback.heuristic() == type);
        REQUIRE(playback.speed() == 125.0f);
        REQUIRE(playback.revision() > revision);
        REQUIRE_FALSE(playback.isPlaying());
        REQUIRE_FALSE(playback.finished());
        REQUIRE_FALSE(playback.found());
        REQUIRE(playback.step() == 0);
        REQUIRE(playback.current() == playback.start());
        REQUIRE(playback.path().empty());
        REQUIRE_FALSE(playback.previous());
        REQUIRE_FALSE(playback.grid().isBlocked(playback.start()));
        REQUIRE_FALSE(playback.grid().isBlocked(playback.goal()));
        for(std::size_t node = 0; node < playback.grid().size(); ++node) {
            REQUIRE(playback.state(node) == (node == playback.start()
                ? GridPlayback::CellState::Frontier : GridPlayback::CellState::Unseen));
            REQUIRE(playback.distance(node) == (node == playback.start()
                ? 0.0 : std::numeric_limits<double>::infinity()));
            REQUIRE_FALSE(playback.onPath(node));
        }

        // A fresh reference search detects stale playback changes or an unreachable endpoint.
        Dijkstra reference(playback.grid().matrix(), playback.start(), playback.goal(), playback.grid().positions(), type);
        const auto& expected = reference.run();
        REQUIRE(expected.found);
        for(std::size_t step = 1; step < expected.steps.size(); ++step) {
            REQUIRE(playback.next());
            requireSnapshot(playback, expected.steps[step]);
        }
        REQUIRE(playback.found());
        REQUIRE(playback.path() == expected.path);
        REQUIRE_FALSE(playback.next());
    }
}

TEST_CASE("Seeded maze generation replaces prior obstacles reproducibly", "[search][grid][playback][maze]") {
    GridPlayback playback(81);
    REQUIRE(playback.setStart(10));
    REQUIRE(playback.setGoal(79));
    REQUIRE(playback.generateMaze(777));
    std::vector<bool> expected;
    for(std::size_t node = 0; node < playback.grid().size(); ++node)
        expected.push_back(playback.grid().isBlocked(node));

    for(std::size_t node = 0; node < playback.grid().size(); ++node)
        playback.setBlocked(node, !expected[node]);
    REQUIRE(playback.generateMaze(777));
    for(std::size_t node = 0; node < playback.grid().size(); ++node)
        REQUIRE(playback.grid().isBlocked(node) == expected[node]);

    // The UI's unseeded entry point must also produce a searchable map.
    REQUIRE(playback.generateMaze());
    for(std::size_t step = 0; step < playback.grid().size() && !playback.finished(); ++step)
        REQUIRE(playback.next());
    REQUIRE(playback.found());
    REQUIRE(playback.path().front() == 10);
    REQUIRE(playback.path().back() == 79);
}

TEST_CASE("Maze generation leaves an active search unchanged", "[search][grid][playback][maze]") {
    GridPlayback playback(100);
    REQUIRE(playback.generateMaze(2345));
    playback.setSpeed(16.0f);
    playback.play();
    playback.update(0.125);
    REQUIRE(playback.isPlaying());
    REQUIRE(playback.step() == 2);

    const auto revision = playback.revision();
    const auto current = playback.current();
    std::vector<bool> obstacles;
    std::vector<double> distances;
    std::vector<GridPlayback::CellState> states;
    for(std::size_t node = 0; node < playback.grid().size(); ++node) {
        obstacles.push_back(playback.grid().isBlocked(node));
        distances.push_back(playback.distance(node));
        states.push_back(playback.state(node));
    }
    REQUIRE_FALSE(playback.generateMaze(9876));
    REQUIRE_FALSE(playback.generateMaze());
    REQUIRE(playback.isPlaying());
    REQUIRE(playback.step() == 2);
    REQUIRE(playback.current() == current);
    REQUIRE(playback.revision() == revision);
    for(std::size_t node = 0; node < playback.grid().size(); ++node) {
        REQUIRE(playback.grid().isBlocked(node) == obstacles[node]);
        REQUIRE(playback.distance(node) == distances[node]);
        REQUIRE(playback.state(node) == states[node]);
    }
    playback.update(0.125);
    REQUIRE(playback.step() == 4);
}

TEST_CASE("Million-cell grid completes with compact search history", "[search][grid][playback][scale]") {
    // Keep the allocation benchmark bounded independently of the configured limit.
    GridPlayback playback(1'000'000);
    REQUIRE(playback.grid().rows() == 1000);
    REQUIRE(playback.grid().columns() == 1000);
    playback.setSpeed(GridPlayback::maxSpeed);
    playback.play();
    playback.update(60.0);
    REQUIRE(playback.step() == 4096);
    while(playback.isPlaying()) playback.update(0.25);
    REQUIRE(playback.found());
    REQUIRE(playback.step() == 1'000'000);
    REQUIRE(playback.distance(playback.goal()) == 1998);
    REQUIRE(playback.path().size() == 1999);
    REQUIRE(playback.previous());
    REQUIRE_FALSE(playback.found());
    REQUIRE(playback.next());
    REQUIRE(playback.found());
    REQUIRE(playback.resize(30));
    REQUIRE(playback.grid().size() == 30);
    REQUIRE(playback.step() == 0);
}
