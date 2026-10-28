#include <iostream>
#include <exception>
#include <config.hpp>

#include <SDL3/SDL_main.h>

#include <window.hpp>
#include <appLayout.hpp>

int main(int, char**) {
    std::cout << project_name << std::endl;
    std::cout << project_version << std::endl;
    std::cout << authors << std::endl;
    std::cout << "C++ Standard: " << __cplusplus << std::endl;

    constexpr int WIDTH = 800;
    constexpr int HEIGHT = 600;

    try {
        Window window(WIDTH, HEIGHT, project_name);
        AppLayout layout;

        while (window.beginFrame()) {
            layout.draw();
            window.endFrame();
        }
    } catch (const std::exception& error) {
        std::cerr << "Application error: " << error.what() << std::endl;
        return 1;
    }

    return 0;
}
