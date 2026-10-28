#pragma once

#include <string_view>
#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;
struct ImGuiContext;
struct ImPlotContext;

class Window {
    public:
        Window(int width, int height, std::string_view title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        void addIcon() const;
        // Poll input and start a frame, waiting while minimized. False means quit.
        bool beginFrame();
        void endFrame();
    private:
        void shutdown() noexcept;

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        ImGuiContext* imguiContext = nullptr;
        ImPlotContext* implotContext = nullptr;
        bool platformInitialized = false;
        bool rendererInitialized = false;
        std::uint64_t frameStart = 0;
};
