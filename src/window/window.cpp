#include <window.hpp>

#include <string_view>

#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

Window::Window(int width, int height, std::string_view title) : width(width), height(height) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    InitWindow(width, height, title.data());
    rlImGuiSetup(true);
}

Window::~Window() {
    rlImGuiShutdown();
    CloseWindow();
}