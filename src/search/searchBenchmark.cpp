#include <searchBenchmark.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
    double prepareGeometry(const Dijkstra::Matrix& graph,
                           std::vector<Heuristics::Position>& positions) {
        const auto count = graph.size();
        const auto columns = static_cast<std::size_t>(std::ceil(std::sqrt(static_cast<long double>(count))));
        positions.reserve(count);
        for(std::size_t node = 0; node < count; ++node) {
            positions.push_back({static_cast<double>(node / columns),
                                 static_cast<double>(node % columns)});
        }

        double edgeScale = std::numeric_limits<double>::infinity();
        for(std::size_t from = 0; from < count; ++from) {
            for(std::size_t to = 0; to < count; ++to) {
                const auto weight = graph[from][to];
                if(from == to || !std::isfinite(weight)) continue;
                const auto distance = Heuristics::manhattan(positions[from], positions[to]);
                edgeScale = std::min(edgeScale, weight / distance);
            }
        }

        // No connecting edges need no estimate. Half the edge-derived scale
        // leaves room for floating-point rounding in the consistency checks.
        double scale = std::isfinite(edgeScale) ? edgeScale * 0.5 : 0.0;
        const auto extent = static_cast<double>((count - 1) / columns) + static_cast<double>(columns - 1);
        if(extent > 0.0) {
            // Also keep coordinates and every possible heuristic finite when
            // edge weights approach the largest representable double.
            scale = std::min(scale, std::numeric_limits<double>::max() / extent / 4.0);
        }
        for(auto& position : positions) {
            position.row *= scale;
            position.column *= scale;
        }
        return scale;
    }
}

void SearchBenchmark::load(Matrix graph) {
    const auto nextGoal = graph.empty() ? 0 : graph.size() - 1;
    // Reuse the algorithm's validation without executing it. Keep all fallible
    // preparation local so malformed input cannot replace an existing graph.
    const Dijkstra validation(graph, 0, nextGoal);
    std::vector<Heuristics::Position> nextPositions;
    const auto nextScale = prepareGeometry(graph, nextPositions);

    this->graph = std::move(graph);
    this->positions = std::move(nextPositions);
    this->startNode = 0;
    this->goalNode = nextGoal;
    this->scale = nextScale;
    this->measurements.reset();
}

bool SearchBenchmark::setStart(std::size_t node) {
    if(node >= this->graph.size()) return false;
    if(node != this->startNode) {
        this->startNode = node;
        this->measurements.reset();
    }
    return true;
}

bool SearchBenchmark::setGoal(std::size_t node) {
    if(node >= this->graph.size()) return false;
    if(node != this->goalNode) {
        this->goalNode = node;
        this->measurements.reset();
    }
    return true;
}

void SearchBenchmark::compare() {
    if(!this->loaded()) throw std::logic_error("Load a graph before starting the comparison.");

    constexpr std::array types{Heuristics::Type::None, Heuristics::Type::Euclidean, Heuristics::Type::Manhattan};
    std::array searches{
        Dijkstra(this->graph, this->startNode, this->goalNode),
        Dijkstra(this->graph, this->startNode, this->goalNode, this->positions, types[1]),
        Dijkstra(this->graph, this->startNode, this->goalNode, this->positions, types[2])
    };
    std::array<Measurement, 3> nextMeasurements;
    for(std::size_t index = 0; index < searches.size(); ++index) {
        const auto begin = std::chrono::steady_clock::now();
        const auto& result = searches[index].run(false);
        const auto end = std::chrono::steady_clock::now();
        nextMeasurements[index].heuristic = types[index];
        nextMeasurements[index].milliseconds = std::chrono::duration<double, std::milli>(end - begin).count();
        nextMeasurements[index].result = result;
    }
    this->measurements = std::move(nextMeasurements);
}

bool SearchBenchmark::loaded() const {
    return !this->graph.empty();
}

bool SearchBenchmark::hasResults() const {
    return this->measurements.has_value();
}

std::size_t SearchBenchmark::nodeCount() const {
    return this->graph.size();
}

std::size_t SearchBenchmark::start() const {
    return this->startNode;
}

std::size_t SearchBenchmark::goal() const {
    return this->goalNode;
}

double SearchBenchmark::geometryScale() const {
    return this->scale;
}

const std::array<SearchBenchmark::Measurement, 3>& SearchBenchmark::results() const {
    if(!this->measurements) throw std::logic_error("The benchmark has no completed comparison.");
    return *this->measurements;
}
