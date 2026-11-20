#include <chromaticPolynomial.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <sstream>
#include <stdexcept>

namespace coloring {
namespace {
// One bit for each unordered pair. Keep n separately so isolates survive both branches.
using EdgeMask = std::uint64_t;
using Polynomial = std::vector<std::int64_t>;
EdgeMask edgeBit(int a, int b) {
    if (a > b) std::swap(a, b);
    return EdgeMask{1} << (b * (b - 1) / 2 + a);
}

EdgeMask contract(EdgeMask remaining, int n, int keep, int remove) {
    auto merged = [=](int v) { return v == remove ? keep : v - (v > remove); };
    EdgeMask result = 0;
    for (int b = 1; b < n; ++b) {
        for (int a = 0; a < b; ++a) {
            // The contracted edge was removed. OR consolidates new parallel edges.
            if (remaining & edgeBit(a, b)) result |= edgeBit(merged(a), merged(b));
        }
    }
    return result;
}

struct Solver {
    bool recordTrace;
    int calls = 0;
    std::ostringstream trace;
    std::map<std::pair<int, EdgeMask>, Polynomial> memo;

    Polynomial solve(int n, EdgeMask edges, int depth, const char* branch) {
        if (++calls > 100000) throw std::runtime_error("Exact analysis reached 100,000 calls. Try a smaller graph.");
        if (recordTrace) trace << std::string(depth * 2, ' ') << branch << " (n=" << n << ") ";
        if (!edges) {
            if (recordTrace) trace << "= q^" << n << '\n';
            Polynomial base(n + 1);
            base[n] = 1;
            return base;
        }
        const auto key = std::pair{n, edges};
        if (const auto hit = memo.find(key); hit != memo.end()) {
            if (recordTrace) trace << "[cached]\n";
            return hit->second;
        }
        for (int b = n - 1; b > 0; --b) {
            for (int a = b - 1; a >= 0; --a) {
                const EdgeMask bit = edgeBit(a, b);
                if (!(edges & bit)) continue;
                if (recordTrace) trace << "e=(" << a << "," << b << ")\n";
                const EdgeMask deleted = edges ^ bit;
                auto result = solve(n, deleted, depth + 1, "G-e");
                const auto contracted = solve(n - 1, contract(deleted, n, a, b), depth + 1, "G/e");
                for (std::size_t i = 0; i < contracted.size(); ++i) result[i] -= contracted[i];
                memo.emplace(key, result);
                return result;
            }
        }
        throw std::logic_error("Invalid edge mask.");
    }
};
}

std::int64_t ExactResult::evaluate(int k) const {
    if (k < 1 || k > maxColors) throw std::invalid_argument("Exact evaluation supports k from 1 to 64.");
    // For n <= 10 and k <= 64 even the unsigned coefficient sum at k
    // is bounded by the complete graph's 64 * 65 * ... * 73, within int64.
    std::int64_t value = 0;
    for (auto i = coefficients.rbegin(); i != coefficients.rend(); ++i) value = value * k + *i;
    return value;
}

std::string ExactResult::formula() const {
    std::ostringstream out;
    bool first = true;
    for (int degree = static_cast<int>(coefficients.size()) - 1; degree >= 0; --degree) {
        const auto c = coefficients[degree];
        if (!c) continue;
        if (!first) out << (c < 0 ? " - " : " + ");
        else if (c < 0) out << "-";
        const auto magnitude = c < 0 ? -c : c;
        if (magnitude != 1 || degree == 0) out << magnitude;
        if (degree > 0) out << "q";
        if (degree > 1) out << "^" << degree;
        first = false;
    }
    return out.str();
}

ExactResult analyze(const Graph& graph) {
    const int n = static_cast<int>(graph.labels.size());
    if (n < 1 || n > maxExactVertices)
        throw std::invalid_argument("Exact analysis supports graphs with 1 to 10 vertices.");
    const auto start = std::chrono::steady_clock::now();
    EdgeMask edges = 0;
    for (auto [a, b] : graph.edges) edges |= edgeBit(a, b);
    Solver solver;
    solver.recordTrace = n <= 4;
    ExactResult result;
    result.coefficients = solver.solve(n, edges, 0, "G");
    result.calls = solver.calls;
    result.chromaticNumber = 1;
    while (result.evaluate(result.chromaticNumber) == 0) ++result.chromaticNumber;
    result.trace = solver.trace.str();
    result.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return result;
}
} // namespace coloring
