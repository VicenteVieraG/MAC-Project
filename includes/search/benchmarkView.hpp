#pragma once

#include <searchBenchmark.hpp>

#include <array>
#include <string>
#include <utility>
#include <vector>

/// File-based timing comparison, independent of graph and grid playback.
class BenchmarkView {
    public:
        void draw();

    private:
        void drawControls();
        void drawResults();
        void drawFilePicker();
        bool loadFile(const std::string& path);

        SearchBenchmark benchmark;
        std::array<char, 4096> filePath{};
        std::array<char, 4096> directoryPath{};
        std::vector<std::pair<std::string, bool>> directoryEntries;
        std::string loadedName;
        std::string errorMessage;
        std::string browseError;
        bool refreshDirectory = true;
};
