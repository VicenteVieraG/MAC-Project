#pragma once

#include <gridPlayback.hpp>

#include <memory>
#include <optional>

class GridView {
    public:
        GridView();
        ~GridView();
        GridView(const GridView&) = delete;
        GridView& operator=(const GridView&) = delete;
        void draw();

    private:
        struct TextureData;
        void drawSidebar();
        void drawCanvas();
        void refreshTexture();
        void paint(std::size_t node);
        void setZoom(float value);
        static constexpr float minZoom = 0.25f;
        static constexpr float maxZoom = 64.0f;

        GridPlayback playback;
        std::unique_ptr<TextureData> texture;
        int requestedCells = 900;
        int editTool = 0;
        float zoom = 1.0f;
        float panX = 0.0f;
        float panY = 0.0f;
        std::optional<std::size_t> lastPainted;
};
