#include <graphPlayback.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <utility>

void GraphPlayback::load(Matrix graph, std::size_t start, std::optional<std::size_t> goal) {
    if(graph.size() > maxNodes)
        throw std::invalid_argument("Graph visualization supports at most 512 nodes.");
    const auto destination = goal.value_or(graph.empty() ? 0 : graph.size() - 1);
    Dijkstra search(graph, start, destination);
    const auto began = std::chrono::steady_clock::now();
    const auto& calculated = search.run();
    const auto elapsed = std::chrono::steady_clock::now() - began;
    // Copy the complete run before changing live state. Invalid input, path-cost
    // overflow, or failed allocation must not discard the previous visualization.
    auto nextRun = calculated;
    this->matrix.swap(graph);
    this->run = std::move(nextRun);
    this->startNode = start;
    this->goalNode = destination;
    this->searchMilliseconds = std::chrono::duration<double, std::milli>(elapsed).count();
    this->restart();
}

bool GraphPlayback::setStart(std::size_t node) {
    if(!this->loaded() || this->playing || node == this->startNode) return false;
    this->load(this->matrix, node, this->goalNode);
    return true;
}

bool GraphPlayback::setGoal(std::size_t node) {
    if(!this->loaded() || this->playing || node == this->goalNode) return false;
    this->load(this->matrix, this->startNode, node);
    return true;
}

void GraphPlayback::play() noexcept {
    if(this->loaded() && !this->finished()) this->playing = true;
}

void GraphPlayback::pause() noexcept {
    this->playing = false;
    this->accumulatedSteps = 0.0;
}

void GraphPlayback::restart() noexcept {
    this->pause();
    this->cursor = 0;
}

bool GraphPlayback::advance() noexcept {
    if(!this->loaded() || this->finished()) return false;
    ++this->cursor;
    if(this->finished()) this->pause();
    return true;
}

bool GraphPlayback::next() noexcept {
    this->pause();
    return this->advance();
}

bool GraphPlayback::previous() noexcept {
    this->pause();
    if(this->cursor == 0) return false;
    --this->cursor;
    return true;
}

void GraphPlayback::update(double elapsedSeconds) noexcept {
    if(!this->playing || !std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0) return;
    this->accumulatedSteps += std::min(elapsedSeconds, 0.25) * this->stepsPerSecond;
    const auto budget = static_cast<std::size_t>(this->accumulatedSteps);
    this->accumulatedSteps -= static_cast<double>(budget);
    for(std::size_t step = 0; step < budget && this->playing; ++step) this->advance();
}

void GraphPlayback::setSpeed(float stepsPerSecond) {
    if(!std::isfinite(stepsPerSecond)) throw std::invalid_argument("Playback speed must be finite.");
    this->stepsPerSecond = std::clamp(stepsPerSecond, minSpeed, maxSpeed);
}

const Dijkstra::SearchStep& GraphPlayback::state() const {
    return this->result().steps[this->cursor];
}

const Dijkstra::SearchResult& GraphPlayback::result() const {
    if(!this->loaded()) throw std::logic_error("Load a graph before inspecting search state.");
    return *this->run;
}
