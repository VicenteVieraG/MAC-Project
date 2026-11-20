#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace coloring {
    inline constexpr int maxVertices = 64;
    inline constexpr int maxColors = 64;

    /// Vertex indices follow input order. Each edge appears once in edges and in both neighbor lists.
    struct Graph {
        std::vector<std::string> labels;
        std::vector<std::vector<int>> neighbors;
        std::vector<std::pair<int, int>> edges;
    };

    /// Parse a simple undirected graph from the counts, vertex IDs and endpoint pairs.
    /// Throws std::invalid_argument for malformed input or unsupported graph sizes.
    Graph parseGraph(const std::string& text);
    /// Read graph text from a regular file of up to 1 MiB for use with parseGraph().
    std::string readGraphFile(const std::filesystem::path& path);
    /// Return k distinct, space-separated names in priority order or throw std::invalid_argument.
    std::vector<std::string> parsePalette(int k, const std::string& names);

} // namespace coloring
