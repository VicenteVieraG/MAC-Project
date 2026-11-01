#include <heuristics.hpp>

#include <cmath>
#include <stdexcept>

namespace {
    void validate(Heuristics::Position position) {
        if(!std::isfinite(position.row) || !std::isfinite(position.column))
            throw std::invalid_argument("Heuristic coordinates must be finite.");
    }

    double checkedDistance(double distance) {
        if(!std::isfinite(distance))
            throw std::overflow_error("Heuristic distance exceeds the finite double range.");
        return distance;
    }
}

double Heuristics::manhattan(Position node, Position goal) {
    validate(node);
    validate(goal);
    return checkedDistance(std::abs(node.row - goal.row) + std::abs(node.column - goal.column));
}

double Heuristics::euclidean(Position node, Position goal) {
    validate(node);
    validate(goal);
    return checkedDistance(std::hypot(node.row - goal.row, node.column - goal.column));
}

double Heuristics::estimate(Type type, Position node, Position goal) {
    validate(node);
    validate(goal);
    switch(type) {
        case Type::None: return 0.0;
        case Type::Manhattan: return manhattan(node, goal);
        case Type::Euclidean: return euclidean(node, goal);
    }
    throw std::invalid_argument("Unknown heuristic type.");
}
