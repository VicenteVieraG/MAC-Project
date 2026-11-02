#pragma once

#include <grid.hpp>
#include <heap.hpp>

#include <cstdint>
#include <limits>

/// Incremental search with reversible playback, independent of rendering.
/// Storage is O(cells + successful relaxations), without per-step snapshots.
class GridPlayback {
    public:
        static constexpr int minCells = 30;
        static constexpr int maxCells = 10'000'000;
        static constexpr float minSpeed = 1.0f;
        static constexpr float maxSpeed = 100'000.0f;
        enum class CellState : std::uint8_t { Unseen, Frontier, Visited };

        explicit GridPlayback(std::size_t cells = 900);
        void play();
        void pause() noexcept;
        void restart();
        bool next();
        bool previous();
        /// At most 4096 steps per frame, dropping excess accumulated time.
        void update(double elapsedSeconds);
        void setSpeed(float stepsPerSecond);

        /// Edits return false while playing; successful edits reset the search.
        bool resize(std::size_t cells);
        bool setBlocked(std::size_t node, bool blocked);
        bool setStart(std::size_t node);
        bool setGoal(std::size_t node);
        bool setHeuristic(Heuristics::Type type);
        bool clearObstacles();

        [[nodiscard]] const Grid& grid() const noexcept { return this->map; }
        [[nodiscard]] std::size_t start() const noexcept { return this->startNode; }
        [[nodiscard]] std::size_t goal() const noexcept { return this->goalNode; }
        [[nodiscard]] bool isPlaying() const noexcept { return this->playing; }
        [[nodiscard]] bool finished() const noexcept { return this->generatedDone && this->cursor == this->history.size(); }
        [[nodiscard]] bool found() const noexcept { return this->finished() && this->reachedGoal; }
        [[nodiscard]] std::size_t step() const noexcept { return this->cursor; }
        [[nodiscard]] std::size_t current() const noexcept;
        [[nodiscard]] float speed() const noexcept { return this->stepsPerSecond; }
        [[nodiscard]] Heuristics::Type heuristic() const noexcept { return this->selectedHeuristic; }
        [[nodiscard]] CellState state(std::size_t node) const { return this->states.at(node); }
        [[nodiscard]] double distance(std::size_t node) const { return this->distances.at(node); }
        [[nodiscard]] bool onPath(std::size_t node) const { return this->found() && this->pathCells.at(node); }
        [[nodiscard]] const std::vector<std::size_t>& path() const noexcept;
        [[nodiscard]] std::size_t revision() const noexcept { return this->visualRevision; }

    private:
        struct Change {
            std::size_t node;
            double previousDistance;
            std::size_t previousPredecessor;
        };
        struct Step {
            std::size_t node;
            std::size_t changeBegin;
            std::size_t changeEnd;
        };

        bool advance();
        void apply(const Step& step);
        Grid map;
        std::size_t startNode = 0;
        std::size_t goalNode;
        Heuristics::Type selectedHeuristic = Heuristics::Type::Manhattan;
        Heap frontier;
        std::vector<double> distances;
        std::vector<std::size_t> predecessors;
        std::vector<CellState> states;
        std::vector<bool> pathCells;
        std::vector<std::size_t> finalPath;
        std::vector<Step> history;
        std::vector<Change> changes;
        std::size_t cursor = 0;
        std::size_t visualRevision = 0;
        float stepsPerSecond = 60.0f;
        double accumulatedSteps = 0.0;
        bool playing = false;
        bool generatedDone = false;
        bool reachedGoal = false;
};
