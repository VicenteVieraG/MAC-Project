#include <grid.hpp>

#include <cmath>
#include <limits>
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

std::size_t Grid::nodeId(std::size_t row, std::size_t column) const {
    if(row >= this->rowCount || column >= this->columnCount)
        throw std::out_of_range("Grid coordinates must be inside the map.");
    const auto node = row * this->columnCount + column;
    if(node >= this->size()) throw std::out_of_range("Grid coordinates must be inside the map.");
    return node;
}
