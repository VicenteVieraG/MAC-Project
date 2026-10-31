#include <dijkstra.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

Dijkstra::Dijkstra(Matrix graph, NodeId start, NodeId goal): graph(std::move(graph)), start(start), goal(goal) {
    const auto count = this->graph.size();
    if(count == 0) throw std::invalid_argument("Dijkstra requires a nonempty graph.");

    for(const auto& row : this->graph) {
        if(row.size() != count) throw std::invalid_argument("Dijkstra requires a square adjacency matrix.");

        for(const auto weight : row) {
            if(std::isnan(weight) || weight < 0.0)
                throw std::invalid_argument("Dijkstra requires nonnegative weights or positive infinity.");
        }
    }

    if(start >= count || goal >= count) throw std::out_of_range("Dijkstra start and goal must be valid node IDs.");
}

const Dijkstra::SearchResult& Dijkstra::run(bool recordSteps) {
    const auto count = this->graph.size();
    SearchResult nextResult;
    SearchStep state{
        this->start,
        std::vector<double>(count, std::numeric_limits<double>::infinity()),
        std::vector<NodeId>(count, NodeId{}),
        std::vector<bool>(count, false),
        std::vector<bool>(count, false)
    };
    Heap frontier;
    state.distances[this->start] = 0.0;
    state.inFrontier[this->start] = true;
    frontier.push(this->start, 0.0);
    if(recordSteps) nextResult.steps.push_back(state);

    while(!frontier.empty()) {
        // A node's cost becomes final only when it is removed from the min-heap.
        const auto current = frontier.pop().node;
        state.current = current;
        state.inFrontier[current] = false;
        state.visited[current] = true;
        ++nextResult.exploredNodes;

        if(current == this->goal) {
            nextResult.found = true;
            nextResult.totalCost = state.distances[current];

            // Stop at the start explicitly: zero is a valid node ID, not a sentinel.
            for(NodeId node = current; node != this->start; node = state.predecessors[node])
                nextResult.path.push_back(node);
            nextResult.path.push_back(this->start);
            std::reverse(nextResult.path.begin(), nextResult.path.end());

            if(recordSteps) nextResult.steps.push_back(state);
            break;
        }

        for(NodeId neighbor = 0; neighbor < count; ++neighbor) {
            const auto weight = this->graph[current][neighbor];
            if(state.visited[neighbor] || std::isinf(weight)) continue;

            const auto candidate = state.distances[current] + weight;
            if(!std::isfinite(candidate))
                throw std::overflow_error("Dijkstra path cost exceeds the finite double range.");
            if(candidate >= state.distances[neighbor]) continue;

            // Relax the edge; keep only one heap entry for each frontier node.
            state.distances[neighbor] = candidate;
            state.predecessors[neighbor] = current;
            if(frontier.contains(neighbor)) {
                frontier.decreaseKey(neighbor, candidate);
            } else {
                frontier.push(neighbor, candidate);
                state.inFrontier[neighbor] = true;
            }
        }

        // Copy vectors by value so later relaxations cannot alter playback history.
        if(recordSteps) nextResult.steps.push_back(state);
    }

    this->lastResult = std::move(nextResult);
    return *this->lastResult;
}

const Dijkstra::SearchResult& Dijkstra::result() const {
    if(!this->lastResult)
        throw std::logic_error("Dijkstra has not completed a run yet.");
    return *this->lastResult;
}
