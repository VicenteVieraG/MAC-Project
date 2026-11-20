#include <coloringGraph.hpp>

#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace coloring {
namespace {
std::vector<std::string> words(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> result;
    for (std::string word; input >> word;) result.push_back(word);
    return result;
}
}

Graph parseGraph(const std::string& text) {
    if (text.find('\0') != std::string::npos) throw std::invalid_argument("Choose a plain-text graph.");
    std::istringstream input(text);
    std::string line;
    std::getline(input, line);
    std::istringstream header(line);
    int n, m;
    std::string extra;
    if (!(header >> n >> m) || header >> extra || words(line).size() != 2 || n < 0 || m < 0)
        throw std::invalid_argument("Line 1 must contain two nonnegative integers: vertices and edges.");
    if (n == 0) throw std::invalid_argument("An empty graph is not supported. Enter at least one vertex.");
    if (n > maxVertices) throw std::invalid_argument("The graph view supports up to 64 vertices.");
    if (m > n * (n - 1) / 2) throw std::invalid_argument("Too many edges for a simple undirected graph.");

    if (!std::getline(input, line)) throw std::invalid_argument("Line 2 must list the vertex IDs.");
    Graph graph;
    graph.labels = words(line);
    if (static_cast<int>(graph.labels.size()) != n)
        throw std::invalid_argument("Line 2 must list exactly " + std::to_string(n) + " vertex IDs.");
    std::map<std::string, int> ids;
    for (int v = 0; v < n; ++v) {
        if (!ids.emplace(graph.labels[v], v).second)
            throw std::invalid_argument("Repeated vertex ID: " + graph.labels[v]);
    }
    graph.neighbors.resize(n);
    for (int edge = 0; edge < m; ++edge) {
        if (!std::getline(input, line)) throw std::invalid_argument("Missing edge " + std::to_string(edge + 1) + ".");
        const auto endpoints = words(line);
        const auto prefix = "Line " + std::to_string(edge + 3) + ": ";
        if (endpoints.size() != 2) throw std::invalid_argument(prefix + "enter two endpoints.");
        if (!ids.contains(endpoints[0]) || !ids.contains(endpoints[1]))
            throw std::invalid_argument(prefix + "both endpoints must appear in the vertex list.");
        const int a = ids.at(endpoints[0]), b = ids.at(endpoints[1]);
        if (a == b) throw std::invalid_argument(prefix + "a vertex cannot connect to itself.");
        if (std::ranges::find(graph.neighbors[a], b) != graph.neighbors[a].end())
            throw std::invalid_argument(prefix + "this undirected edge already exists.");
        graph.edges.emplace_back(a, b);
        graph.neighbors[a].push_back(b);
        graph.neighbors[b].push_back(a);
    }
    while (std::getline(input, line))
        if (!words(line).empty()) throw std::invalid_argument("Extra data after the declared edge list.");
    return graph;
}

std::string readGraphFile(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path)) throw std::runtime_error("Choose an existing graph file.");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Could not open the graph file.");
    constexpr std::size_t maxBytes = 1024 * 1024;
    std::string text(maxBytes + 1, '\0');
    input.read(text.data(), static_cast<std::streamsize>(text.size()));
    if (input.bad()) throw std::runtime_error("Could not read the graph file.");
    text.resize(static_cast<std::size_t>(input.gcount()));
    if (text.size() > maxBytes) throw std::invalid_argument("Graph files must be 1 MiB or smaller.");
    return text;
}

std::vector<std::string> parsePalette(int k, const std::string& names) {
    if (k < 1 || k > maxColors) throw std::invalid_argument("Choose between 1 and 64 available colors.");
    auto palette = words(names);
    if (static_cast<int>(palette.size()) != k)
        throw std::invalid_argument("Enter exactly " + std::to_string(k) + " color names in priority order.");
    if (std::set<std::string>(palette.begin(), palette.end()).size() != palette.size())
        throw std::invalid_argument("Each color needs a unique name.");
    return palette;
}
} // namespace coloring
