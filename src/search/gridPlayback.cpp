#include <gridPlayback.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
    std::size_t checkedCount(std::size_t count) {
        if(count < GridPlayback::minCells || count > GridPlayback::maxCells)
            throw std::out_of_range("Grid size must be between 30 and 10,000,000 cells.");
        return count;
    }
}

GridPlayback::GridPlayback(std::size_t cells) : map(Grid::fromCellCount(checkedCount(cells))), goalNode(cells - 1) {
    this->restart();
}

void GridPlayback::play() {
    if(!this->finished()) this->playing = true;
}

void GridPlayback::pause() noexcept {
    this->playing = false;
    this->accumulatedSteps = 0.0;
}

void GridPlayback::restart() {
    this->pause();
    this->frontier.clear();
    this->history.clear();
    this->changes.clear();
    this->finalPath.clear();
    this->cursor = 0;
    this->generatedDone = false;
    this->reachedGoal = false;
    this->distances.assign(this->map.size(), std::numeric_limits<double>::infinity());
    this->predecessors.assign(this->map.size(), this->map.size());
    this->states.assign(this->map.size(), CellState::Unseen);
    this->pathCells.assign(this->map.size(), false);
    this->distances[this->startNode] = 0.0;
    this->states[this->startNode] = CellState::Frontier;
    this->frontier.push(this->startNode, Heuristics::estimate(this->selectedHeuristic,
        this->map.positions()[this->startNode], this->map.positions()[this->goalNode]));
    ++this->visualRevision;
}

void GridPlayback::apply(const Step& step) {
    this->states[step.node] = CellState::Visited;
    for(auto index = step.changeBegin; index < step.changeEnd; ++index) {
        const auto node = this->changes[index].node;
        this->distances[node] = this->distances[step.node] + 1.0;
        this->predecessors[node] = step.node;
        this->states[node] = CellState::Frontier;
    }
}

bool GridPlayback::advance() {
    if(this->finished()) return false;
    if(this->cursor < this->history.size()) {
        this->apply(this->history[this->cursor]);
    } else {
        const auto node = this->frontier.pop().node;
        Step step{node, this->changes.size(), this->changes.size()};
        this->states[node] = CellState::Visited;
        if(node == this->goalNode) {
            this->generatedDone = true;
            this->reachedGoal = true;
            for(auto cell = node; ; cell = this->predecessors[cell]) {
                this->finalPath.push_back(cell);
                this->pathCells[cell] = true;
                if(cell == this->startNode) break;
            }
            std::reverse(this->finalPath.begin(), this->finalPath.end());
        } else {
            for(const auto neighbor : this->map.neighbors(node)) {
                if(neighbor == this->map.size() || this->states[neighbor] == CellState::Visited) continue;
                const auto cost = this->distances[node] + 1.0;
                if(cost >= this->distances[neighbor]) continue;
                this->changes.push_back({neighbor, this->distances[neighbor], this->predecessors[neighbor]});
                this->distances[neighbor] = cost;
                this->predecessors[neighbor] = node;
                this->states[neighbor] = CellState::Frontier;
                const auto priority = cost + Heuristics::estimate(this->selectedHeuristic,
                    this->map.positions()[neighbor], this->map.positions()[this->goalNode]);
                if(this->frontier.contains(neighbor)) this->frontier.decreaseKey(neighbor, priority);
                else this->frontier.push(neighbor, priority);
            }
            this->generatedDone = this->frontier.empty();
        }
        step.changeEnd = this->changes.size();
        this->history.push_back(step);
    }
    ++this->cursor;
    ++this->visualRevision;
    if(this->finished()) this->pause();
    return true;
}

bool GridPlayback::next() {
    this->pause();
    return this->advance();
}

bool GridPlayback::previous() {
    this->pause();
    if(this->cursor == 0) return false;
    const auto& step = this->history[--this->cursor];
    for(auto index = step.changeEnd; index > step.changeBegin; ) {
        const auto& change = this->changes[--index];
        this->distances[change.node] = change.previousDistance;
        this->predecessors[change.node] = change.previousPredecessor;
        this->states[change.node] = std::isfinite(change.previousDistance) ? CellState::Frontier : CellState::Unseen;
    }
    this->states[step.node] = CellState::Frontier;
    // Keep the heap at the furthest generated step; replay changes before
    // generating any new steps. Editing discards both the heap and history.
    ++this->visualRevision;
    return true;
}

void GridPlayback::update(double elapsedSeconds) {
    if(!this->playing || !std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0) return;
    this->accumulatedSteps = std::min(4096.0,
        this->accumulatedSteps + std::min(elapsedSeconds, 0.25) * this->stepsPerSecond);
    const auto budget = static_cast<int>(this->accumulatedSteps);
    this->accumulatedSteps -= budget;
    for(int step = 0; step < budget && this->playing; ++step) this->advance();
}

void GridPlayback::setSpeed(float stepsPerSecond) {
    if(!std::isfinite(stepsPerSecond)) throw std::invalid_argument("Playback speed must be finite.");
    this->stepsPerSecond = std::clamp(stepsPerSecond, minSpeed, maxSpeed);
}

bool GridPlayback::resize(std::size_t cells) {
    if(this->playing || cells == this->map.size()) return false;
    this->map = Grid::fromCellCount(checkedCount(cells));
    this->startNode = 0;
    this->goalNode = cells - 1;
    this->restart();
    return true;
}

bool GridPlayback::setBlocked(std::size_t node, bool blocked) {
    if(this->playing || node == this->startNode || node == this->goalNode) return false;
    if(this->map.isBlocked(node) == blocked) return false;
    this->map.setBlocked(node, blocked);
    // Painting multiple cells before the first step needs no search reallocation.
    if(!this->history.empty()) this->restart();
    else ++this->visualRevision;
    return true;
}

bool GridPlayback::setStart(std::size_t node) {
    if(this->playing || node == this->startNode || node == this->goalNode) return false;
    this->map.setBlocked(node, false);
    this->startNode = node;
    this->restart();
    return true;
}

bool GridPlayback::setGoal(std::size_t node) {
    if(this->playing || node == this->goalNode || node == this->startNode) return false;
    this->map.setBlocked(node, false);
    this->goalNode = node;
    this->restart();
    return true;
}

bool GridPlayback::setHeuristic(Heuristics::Type type) {
    if(this->playing || type == this->selectedHeuristic) return false;
    (void)Heuristics::estimate(type, {}, {});
    this->selectedHeuristic = type;
    this->restart();
    return true;
}

bool GridPlayback::clearObstacles() {
    if(this->playing) return false;
    for(std::size_t node = 0; node < this->map.size(); ++node)
        if(this->map.isBlocked(node)) this->map.setBlocked(node, false);
    this->restart();
    return true;
}

std::size_t GridPlayback::current() const noexcept {
    return this->cursor == 0 ? this->startNode : this->history[this->cursor - 1].node;
}

const std::vector<std::size_t>& GridPlayback::path() const noexcept {
    static const std::vector<std::size_t> empty;
    return this->found() ? this->finalPath : empty;
}
