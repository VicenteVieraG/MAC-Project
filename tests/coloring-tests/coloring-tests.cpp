#include <coloringGraph.hpp>
#include <chromaticPolynomial.hpp>
#include <coloringRun.hpp>

#include <algorithm>
#include <set>
#include <catch2/catch_test_macros.hpp>

using namespace coloring;

namespace {
Graph example(const char* file) {
    return parseGraph(readGraphFile(std::filesystem::path(COLORING_DATA_DIR) / file));
}

void checkRun(const Graph& graph, int k, const Run& result) {
    std::set<int> assigned, used;
    for (int v : result.order) {
        REQUIRE(assigned.insert(v).second);
        REQUIRE(result.colors[v] >= 0);
        REQUIRE(result.colors[v] < k);
        used.insert(result.colors[v]);
    }
    REQUIRE(used.size() == static_cast<std::size_t>(result.used));
    for (auto [a, b] : graph.edges)
        if (assigned.contains(a) && assigned.contains(b)) REQUIRE(result.colors[a] != result.colors[b]);
    if (result.success()) REQUIRE(assigned.size() == graph.labels.size());
    else {
        REQUIRE(result.colors[result.failedVertex] == -1);
        std::set<int> blocked;
        for (int v : graph.neighbors[result.failedVertex])
            if (assigned.contains(v)) blocked.insert(result.colors[v]);
        REQUIRE(static_cast<int>(blocked.size()) == k);
    }
}
}

TEST_CASE("Phase 3 validates edge lists and named palettes", "[coloring]") {
    auto graph = parseGraph("3 1\r\nZ A X\r\nA Z\r\n\n");
    REQUIRE(graph.labels == std::vector<std::string>{"Z", "A", "X"});
    REQUIRE(graph.neighbors[2].empty());
    for (const char* invalid : {"0 0\n", "2 0", "-1 0\n", "3 -1\n", "3 1.5\nA B C\n",
                                "3 0 x\nA B C", "3 0\nA B", "3 0\nA A C", "3 1\nA B C\nA Q",
                                "3 1\nA B C\nA A", "3 2\nA B C\nA B\nB A", "3 2\nA B C\nA B",
                                "3 1\nA B C\nA B C", "3 0\nA B C\nA B", "65 0\n"})
        REQUIRE_THROWS(parseGraph(invalid));
    REQUIRE(parsePalette(2, "Blue Red") == std::vector<std::string>{"Blue", "Red"});
    REQUIRE_THROWS(parsePalette(0, ""));
    REQUIRE_THROWS(parsePalette(65, ""));
    REQUIRE_THROWS(parsePalette(2, "Red Red"));
    REQUIRE_THROWS(parsePalette(2, "Red"));
}

TEST_CASE("Deletion and contraction reproduce the course reference and retain isolates", "[coloring]") {
    const auto result = analyze(example("activity-17.txt"));
    REQUIRE(result.coefficients == std::vector<std::int64_t>{0, -2, 5, -4, 1});
    REQUIRE(result.formula() == "q^4 - 4q^3 + 5q^2 - 2q");
    REQUIRE(result.chromaticNumber == 3);
    REQUIRE(result.evaluate(1) == 0);
    REQUIRE(result.evaluate(2) == 0);
    REQUIRE(result.evaluate(3) == 12);
    REQUIRE(result.evaluate(4) == 72);
    REQUIRE(result.calls > 1);
    REQUIRE(result.trace.find("G-e") != std::string::npos);
    REQUIRE(result.trace.find("G/e") != std::string::npos);
    REQUIRE(analyze(parseGraph("3 1\nC A B\nB A")).evaluate(3) == 18);
    const auto isolates = analyze(parseGraph("10 0\nA B C D E F G H I J"));
    REQUIRE(isolates.chromaticNumber == 1);
    REQUIRE(isolates.evaluate(64) == (std::int64_t{1} << 60));
    REQUIRE_THROWS(analyze(parseGraph("11 0\nA B C D E F G H I J K")));
}

TEST_CASE("Four experiments share the same graph and boundary palettes", "[coloring]") {
    struct Case { const char* file; int chi; int atChi; };
    for (auto [file, chi, expected] : {Case{"activity-17.txt", 3, 12}, Case{"odd-cycle.txt", 3, 30},
                                      Case{"bipartite.txt", 2, 2}, Case{"irregular-tree.txt", 2, 2}}) {
        CAPTURE(file);
        const auto graph = example(file);
        const auto exact = analyze(graph);
        REQUIRE(exact.chromaticNumber == chi);
        REQUIRE(exact.evaluate(chi) == expected);
        for (int k = chi - 1; k <= chi + 1; ++k) {
            for (auto a : {Algorithm::Greedy, Algorithm::WelshPowell, Algorithm::MMI}) {
                for (auto m : {Mode::Basic, Mode::MRV, Mode::LCV, Mode::Both}) {
                    CAPTURE(k, static_cast<int>(a), static_cast<int>(m));
                    const auto r = color(graph, k, a, m);
                    checkRun(graph, k, r);
                    if (exact.evaluate(k) == 0) REQUIRE_FALSE(r.success());
                    const auto replay = color(graph, k, a, m);
                    REQUIRE(replay.order == r.order);
                    REQUIRE(replay.colors == r.colors);
                    REQUIRE(replay.checks == r.checks);
                    REQUIRE((r.mrvSelections > 0) == (m == Mode::MRV || m == Mode::Both));
                    REQUIRE((r.lcvEvaluations > 0) == (m == Mode::LCV || m == Mode::Both));
                }
            }
        }
    }
}

TEST_CASE("Greedy order, smallest-last order and heuristic decisions are observable", "[coloring]") {
    const auto chain = parseGraph("5 4\nA B C D E\nA B\nB C\nC D\nD E");
    REQUIRE(color(chain, 3, Algorithm::Greedy, Mode::Basic).order == std::vector<int>{2, 1, 3, 0, 4});
    REQUIRE(color(chain, 3, Algorithm::WelshPowell, Mode::Basic).order == std::vector<int>{1, 2, 3, 0, 4});
    const auto activity = example("activity-17.txt");
    REQUIRE(color(activity, 3, Algorithm::MMI, Mode::Basic).order == std::vector<int>{2, 1, 0, 3});
    const auto tree = example("irregular-tree.txt");
    const auto basic = color(tree, 2, Algorithm::WelshPowell, Mode::Basic);
    REQUIRE_FALSE(basic.success());
    REQUIRE(basic.failedVertex == 3);
    REQUIRE(basic.order == std::vector<int>{0, 1, 2});
    const auto mrv = color(tree, 2, Algorithm::WelshPowell, Mode::MRV);
    REQUIRE(mrv.success());
    REQUIRE(mrv.mrvTies > 0);

    const auto path = parseGraph("6 5\nA B C D E F\nA B\nA C\nB E\nD E\nD F");
    const auto basePath = color(path, 3, Algorithm::WelshPowell, Mode::Basic);
    const auto lcv = color(path, 3, Algorithm::WelshPowell, Mode::LCV);
    REQUIRE(lcv.order == basePath.order);
    // B already blocks color 1 at E. Assigning 1 to D leaves E's domain intact.
    REQUIRE(basePath.colors[3] == 0);
    REQUIRE(lcv.colors[3] == 1);
    REQUIRE(lcv.lcvChanges > 0);
    REQUIRE(lcv.lcvTies > 0);
    REQUIRE(lcv.checks >= lcv.lcvEvaluations);
}
