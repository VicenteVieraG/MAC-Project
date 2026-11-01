#include <grid.hpp>

#include <limits>
#include <stdexcept>

Grid::Grid(std::size_t sideLength) : sideLength(sideLength) {
    if(sideLength == 0)
        throw std::invalid_argument("Grid side length must be positive.");
    // Check before multiplying so the node count cannot wrap around.
    if(sideLength > std::numeric_limits<std::size_t>::max() / sideLength)
        throw std::overflow_error("Grid node count exceeds the size_t range.");
    const auto nodeCount = sideLength * sideLength;
    this->weights.assign(nodeCount, std::vector<double>(nodeCount, std::numeric_limits<double>::infinity()));
    this->coordinates.reserve(nodeCount);
    this->obstacles.assign(nodeCount, false);

    for(std::size_t row = 0; row < this->sideLength; ++row) {
        for(std::size_t column = 0; column < this->sideLength; ++column) {
            const auto node = row * this->sideLength + column;
            this->coordinates.push_back({static_cast<double>(row), static_cast<double>(column)});
            this->weights[node][node] = 0.0;
            const auto connect = [&](std::size_t neighbor) {
                this->weights[node][neighbor] = 1.0;
                this->weights[neighbor][node] = 1.0;
            };
            // Each pair is added once in both directions. Column bounds prevent
            // wrapping from the right edge of a row into the next row.
            if(column + 1 < this->sideLength) connect(node + 1);
            if(row + 1 < this->sideLength) connect(node + this->sideLength);
        }
    }
}

void Grid::setBlocked(std::size_t node, bool blocked) {
    if(node >= this->obstacles.size())
        throw std::out_of_range("Grid node must be inside the map.");
    this->obstacles[node] = blocked;
    const auto row = node / this->sideLength;
    const auto column = node % this->sideLength;
    const auto update = [&](std::size_t neighbor) {
        const auto cost = blocked || this->obstacles[neighbor]
            ? std::numeric_limits<double>::infinity() : 1.0;
        this->weights[node][neighbor] = cost;
        this->weights[neighbor][node] = cost;
    };
    if(row > 0) update(node - this->sideLength);
    if(row + 1 < this->sideLength) update(node + this->sideLength);
    if(column > 0) update(node - 1);
    if(column + 1 < this->sideLength) update(node + 1);
}

std::size_t Grid::nodeId(std::size_t row, std::size_t column) const {
    if(row >= this->sideLength || column >= this->sideLength)
        throw std::out_of_range("Grid coordinates must be inside the map.");
    return row * this->sideLength + column;
}
