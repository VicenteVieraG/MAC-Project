#pragma once

#include <graphPlayback.hpp>

#include <array>
#include <optional>
#include <string>
#include <vector>

/// Interactive, directed graph preview. Node positions never affect edge costs.
class GraphView {
    public:
        void draw();

    private:
        enum class Layout { Circular, Random };
        struct Position { float x, y; };
        struct Edge { std::size_t from, to; double weight; };
        void drawSidebar();
        void drawCanvas();
        void drawCosts();
        void drawMatrix();
        void drawFilePicker();
        void loadFile(const std::string& path);
        void loadExample();
        void graphLoaded(std::string name);
        void resetLayout();
        void fitGraph();
        void setZoom(float value);
        bool onPath(std::size_t node) const;

        GraphPlayback playback;
        std::vector<Position> positions;
        std::vector<Edge> edges;
        std::vector<std::size_t> incoming, outgoing;
        std::array<char, 4096> filePath{};
        std::array<char, 4096> directoryPath{};
        std::vector<std::pair<std::string, bool>> directoryEntries;
        std::string loadedName;
        std::string loadError;
        std::string browseError;
        bool refreshDirectory = true;
        bool showWeights = true;
        bool showNodeIds = true;
        Layout layout = Layout::Circular;
        float zoom = 1.0f;
        float panX = 0.0f, panY = 0.0f;
        Position viewCenter{0.0f, 0.0f};
        Position viewSpan{600.0f, 600.0f};
        std::optional<std::size_t> draggedNode;
        int matrixPage = 0;
        static constexpr float minZoom = 0.1f;
        static constexpr float maxZoom = 10.0f;
};
