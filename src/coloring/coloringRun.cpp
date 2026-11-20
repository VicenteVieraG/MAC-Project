#include <coloringRun.hpp>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <stdexcept>

namespace coloring {
namespace {
int neighborDegreeSum(const Graph& graph, int v) {
    int sum = 0;
    for (int neighbor : graph.neighbors[v]) sum += static_cast<int>(graph.neighbors[neighbor].size());
    return sum;
}

std::vector<int> vertexOrder(const Graph& graph, Algorithm algorithm) {
    const int n = static_cast<int>(graph.labels.size());
    std::vector<int> order(n), degree(n), neighborDegreeSums(n);
    std::iota(order.begin(), order.end(), 0);
    for (int v : order) {
        degree[v] = static_cast<int>(graph.neighbors[v].size());
        if (algorithm == Algorithm::Greedy) neighborDegreeSums[v] = neighborDegreeSum(graph, v);
    }
    if (algorithm == Algorithm::MMI) {
        // Peel a minimum residual degree vertex; color in reverse removal order.
        std::vector<int> removed;
        while (!order.empty()) {
            auto next = std::min_element(order.begin(), order.end(), [&](int a, int b) {
                if (degree[a] != degree[b]) return degree[a] < degree[b];
                return graph.labels[a] < graph.labels[b];
            });
            const int v = *next;
            removed.push_back(v);
            order.erase(next);
            for (int neighbor : graph.neighbors[v]) --degree[neighbor];
        }
        std::reverse(removed.begin(), removed.end());
        return removed;
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (degree[a] != degree[b]) return degree[a] > degree[b];
        if (neighborDegreeSums[a] != neighborDegreeSums[b]) return neighborDegreeSums[a] > neighborDegreeSums[b];
        return graph.labels[a] < graph.labels[b];
    });
    return order;
}

// A lookup answers whether any assigned neighbor has this color. Assignments
// only remove colors, so the same cache works for base, MRV and LCV.
struct Domains {
    std::vector<std::vector<bool>> blocked;
    int checks = 0;

    bool legal(int v, int c) {
        ++checks;
        return !blocked[v][c];
    }
};

int selectMRV(const std::vector<int>& remaining, int k, Domains& domains, Run& run) {
    ++run.mrvSelections;
    int best = remaining.front(), minimum = k + 1, ties = 0;
    for (int v : remaining) {
        int count = 0;
        for (int c = 0; c < k; ++c) count += domains.legal(v, c);
        if (count < minimum) {
            best = v;
            minimum = count;
            ties = 1;
        } else if (count == minimum) ++ties;
    }
    if (ties > 1) ++run.mrvTies;
    return best;
}

int selectColor(const Graph& graph, int v, int k, bool useLCV, Domains& domains, Run& run) {
    int first = -1, best = -1, minimum = maxVertices + 1, ties = 0;
    for (int c = 0; c < k; ++c) {
        if (!domains.legal(v, c)) continue;
        if (!useLCV) return c;
        if (first == -1) first = c;
        ++run.lcvEvaluations;
        int loss = 0;
        for (int neighbor : graph.neighbors[v]) {
            if (run.colors[neighbor] == -1 && domains.legal(neighbor, c)) ++loss;
        }
        if (loss < minimum) {
            best = c;
            minimum = loss;
            ties = 1;
        } else if (loss == minimum) ++ties;
    }
    if (ties > 1) ++run.lcvTies;
    if (first != best) ++run.lcvChanges;
    return best;
}
}

Run color(const Graph& graph, int k, Algorithm algorithm, Mode mode) {
    if (k < 1 || k > maxColors) throw std::invalid_argument("Choose between 1 and 64 colors.");
    const auto start = std::chrono::steady_clock::now();
    auto remaining = vertexOrder(graph, algorithm);
    const bool useMRV = mode == Mode::MRV || mode == Mode::Both;
    const bool useLCV = mode == Mode::LCV || mode == Mode::Both;
    Run run{.algorithm = algorithm, .mode = mode};
    run.colors.assign(remaining.size(), -1);
    Domains domains{std::vector(remaining.size(), std::vector<bool>(k))};
    std::vector<bool> used(k);
    while (!remaining.empty()) {
        const int v = useMRV ? selectMRV(remaining, k, domains, run) : remaining.front();
        const int c = selectColor(graph, v, k, useLCV, domains, run);
        if (c == -1) {
            run.failedVertex = v;
            break;
        }
        run.colors[v] = c;
        run.order.push_back(v);
        used[c] = true;
        for (int neighbor : graph.neighbors[v]) domains.blocked[neighbor][c] = true;
        std::erase(remaining, v);
    }
    run.used = static_cast<int>(std::count(used.begin(), used.end(), true));
    run.checks = domains.checks;
    run.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return run;
}
} // namespace coloring
