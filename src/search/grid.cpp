#include <grid.hpp>

#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace {
    std::size_t squareCount(std::size_t side) {
        if(side == 0) throw std::invalid_argument("Grid side length must be positive.");
        if(side > std::numeric_limits<std::size_t>::max() / side)
            throw std::overflow_error("Grid node count exceeds the size_t range.");
        return side * side;
    }
}

Grid::Grid(std::size_t sideLength) : Grid(squareCount(sideLength), sideLength) {}

Grid::Grid(std::size_t count, std::size_t columns)
    : rowCount(count / columns + (count % columns != 0)), columnCount(columns), obstacles(count, false) {
    this->coordinates.reserve(count);
    for(std::size_t node = 0; node < count; ++node)
        this->coordinates.push_back({static_cast<double>(node / columns), static_cast<double>(node % columns)});
}

Grid Grid::fromCellCount(std::size_t count) {
    if(count == 0) throw std::invalid_argument("Grid cell count must be positive.");
    auto columns = static_cast<std::size_t>(std::sqrt(static_cast<long double>(count)));
    // Division avoids overflow while rounding up the square root.
    while(columns < count / columns || (columns == count / columns && count % columns != 0)) ++columns;
    return Grid(count, columns);
}

bool Grid::isBlocked(std::size_t node) const {
    if(node >= this->size()) throw std::out_of_range("Grid node must be inside the map.");
    return this->obstacles[node];
}

std::array<std::size_t, 4> Grid::neighbors(std::size_t node) const {
    const auto missing = this->size();
    std::array<std::size_t, 4> result{missing, missing, missing, missing};
    if(this->isBlocked(node)) return result;
    const auto column = node % this->columnCount;
    if(node >= this->columnCount) result[0] = node - this->columnCount;
    if(column > 0) result[1] = node - 1;
    if(column + 1 < this->columnCount && node + 1 < missing) result[2] = node + 1;
    if(this->columnCount < missing - node) result[3] = node + this->columnCount;
    for(auto& neighbor : result)
        if(neighbor != missing && this->obstacles[neighbor]) neighbor = missing;
    return result;
}

const Grid::Matrix& Grid::matrix() const {
    if(this->weights.empty()) {
        Matrix next(this->size(), std::vector<double>(this->size(), std::numeric_limits<double>::infinity()));
        for(std::size_t node = 0; node < this->size(); ++node) {
            next[node][node] = 0.0;
            for(const auto neighbor : this->neighbors(node))
                if(neighbor != this->size()) next[node][neighbor] = 1.0;
        }
        this->weights = std::move(next);
    }
    return this->weights;
}

void Grid::setBlocked(std::size_t node, bool blocked) {
    if(node >= this->size()) throw std::out_of_range("Grid node must be inside the map.");
    this->obstacles[node] = blocked;
    if(this->weights.empty()) return;
    // Preserve references to an already-materialized matrix during obstacle edits.
    const auto update = [&](std::size_t neighbor) {
        const auto cost = blocked || this->obstacles[neighbor] ? std::numeric_limits<double>::infinity() : 1.0;
        this->weights[node][neighbor] = cost;
        this->weights[neighbor][node] = cost;
    };
    const auto column = node % this->columnCount;
    if(node >= this->columnCount) update(node - this->columnCount);
    if(this->columnCount < this->size() - node) update(node + this->columnCount);
    if(column > 0) update(node - 1);
    if(column + 1 < this->columnCount && node + 1 < this->size()) update(node + 1);
}

void Grid::generateMaze(std::size_t start, std::size_t goal, std::uint32_t seed) {
    if(start >= this->size() || goal >= this->size())
        throw std::out_of_range("Maze endpoints must be inside the map.");

    // Even rows and columns hold rooms; the intervening cells hold walls.
    // Build separately so allocation failures cannot leave a half-carved map.
    std::vector<bool> blocked(this->size(), true);
    std::vector<std::size_t> pending{0};
    blocked[0] = false;
    std::mt19937 random(seed);
    const auto columns = this->columnCount;
    struct Passage { std::size_t room; std::size_t wall; };
    while(!pending.empty()) {
        const auto node = pending.back();
        const auto row = node / columns;
        const auto column = node % columns;
        std::array<Passage, 4> candidates{};
        std::size_t count = 0;
        const auto add = [&](std::size_t room, std::size_t wall) {
            if(blocked[room]) candidates[count++] = {room, wall};
        };
        if(row >= 2) add(node - 2 * columns, node - columns);
        if(column >= 2) add(node - 2, node - 1);
        if(columns - column > 2 && this->size() - node > 2) add(node + 2, node + 1);
        // Count actual cells, including a potentially incomplete final row.
        if(columns <= (this->size() - 1 - node) / 2) add(node + 2 * columns, node + columns);
        if(count == 0) {
            pending.pop_back();
            continue;
        }
        const auto passage = candidates[std::uniform_int_distribution<std::size_t>(0, count - 1)(random)];
        blocked[passage.wall] = false;
        blocked[passage.room] = false;
        pending.push_back(passage.room);
    }

    // A custom endpoint can lie on a wall or an even-sized grid's outer edge.
    // Moving up, then left, reaches a carved room in at most two moves and
    // never enters a missing cell of the final row.
    const auto connect = [&](std::size_t node) {
        if(!blocked[node]) return;
        blocked[node] = false;
        if((node / columns) % 2 != 0) {
            node -= columns;
            blocked[node] = false;
        }
        if((node % columns) % 2 != 0) blocked[node - 1] = false;
    };
    connect(start);
    connect(goal);

    // Keep any previously materialized compatibility matrix and its references
    // valid without allocating one for the visualization.
    for(std::size_t node = 0; node < this->size(); ++node)
        this->setBlocked(node, blocked[node]);
}

std::size_t Grid::nodeId(std::size_t row, std::size_t column) const {
    if(row >= this->rowCount || column >= this->columnCount)
        throw std::out_of_range("Grid coordinates must be inside the map.");
    const auto node = row * this->columnCount + column;
    if(node >= this->size()) throw std::out_of_range("Grid coordinates must be inside the map.");
    return node;
}
