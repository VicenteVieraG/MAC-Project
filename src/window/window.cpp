#include <window.hpp>

#include <string_view>

#include <imguiConfig.hpp>
#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

Window::Window(int width, int height, std::string_view title) : width(width), height(height) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    InitWindow(width, height, title.data());
    this->addIcon();

    rlImGuiBeginInitImGui();
        imguiConfig::setup();
    rlImGuiEndInitImGui();
}

Window::~Window() {
    rlImGuiShutdown();
    CloseWindow();
}

void Window::addIcon() const {
    Image icon = LoadImage("assets/images/XtremeDevs.png");

    if(IsImageValid(icon)){
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(icon);
        UnloadImage(icon);
    }else {
        TraceLog(LOG_WARNING, "Icon failed to load. Working directory: %s", GetWorkingDirectory());
    }
}