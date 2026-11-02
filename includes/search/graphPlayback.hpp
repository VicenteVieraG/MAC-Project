#pragma once

#include <dijkstra.hpp>

#include <optional>

/// Reversible playback of the existing Dijkstra implementation, without heuristics.
/// The visualization has a bounded graph size because each explored node owns
/// a full search snapshot. Moving a displayed node never changes edge costs.
class GraphPlayback {
    public:
        using Matrix = Dijkstra::Matrix;
        static constexpr std::size_t maxNodes = 512;
        static constexpr float minSpeed = 1.0f;
        static constexpr float maxSpeed = 1000.0f;

        /// Validate and calculate a run before replacing the current graph.
        /// Any exception preserves the graph, endpoints, playback, and speed.
        void load(Matrix graph, std::size_t start = 0,
                  std::optional<std::size_t> goal = std::nullopt);
        /// Endpoint changes restart playback; return false while playing,
        /// before loading a graph, or when the endpoint is unchanged.
        bool setStart(std::size_t node);
        bool setGoal(std::size_t node);

        void play() noexcept;
        void pause() noexcept;
        void restart() noexcept;
        bool next() noexcept;
        bool previous() noexcept;
        /// Ignore invalid elapsed time and cap catch-up at 0.25 seconds/frame.
        void update(double elapsedSeconds) noexcept;
        void setSpeed(float stepsPerSecond);

        [[nodiscard]] bool loaded() const noexcept { return this->run.has_value(); }
        [[nodiscard]] const Matrix& graph() const noexcept { return this->matrix; }
        [[nodiscard]] std::size_t start() const noexcept { return this->startNode; }
        [[nodiscard]] std::size_t goal() const noexcept { return this->goalNode; }
        [[nodiscard]] bool isPlaying() const noexcept { return this->playing; }
        [[nodiscard]] bool finished() const noexcept { return this->loaded() && this->cursor == this->totalSteps(); }
        [[nodiscard]] bool found() const noexcept { return this->finished() && this->run->found; }
        [[nodiscard]] std::size_t step() const noexcept { return this->cursor; }
        /// Number of explored nodes; step zero is the initial frontier.
        [[nodiscard]] std::size_t totalSteps() const noexcept { return this->loaded() ? this->run->steps.size() - 1 : 0; }
        [[nodiscard]] float speed() const noexcept { return this->stepsPerSecond; }
        /// Computation and snapshot recording time, excluding playback and rendering.
        [[nodiscard]] double elapsedMilliseconds() const noexcept { return this->searchMilliseconds; }
        /// Observers throw std::logic_error until a graph is successfully loaded.
        [[nodiscard]] const Dijkstra::SearchStep& state() const;
        [[nodiscard]] const Dijkstra::SearchResult& result() const;

    private:
        bool advance() noexcept;
        Matrix matrix;
        std::optional<Dijkstra::SearchResult> run;
        std::size_t startNode = 0;
        std::size_t goalNode = 0;
        std::size_t cursor = 0;
        float stepsPerSecond = 2.0f;
        double accumulatedSteps = 0.0;
        double searchMilliseconds = 0.0;
        bool playing = false;
};
