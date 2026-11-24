#pragma once

#include <raylib.h>

class Home {
public:
    Home();
    ~Home();
    Home(const Home&) = delete;
    Home& operator=(const Home&) = delete;

    void draw();

private:
    Model logo{};
    Shader sky{};
    RenderTexture2D canvas{};
    int resolutionLocation{};
    int driftLocation{};
};
