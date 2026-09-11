#include <iostream>
#include <config.hpp>

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

#include <window.hpp>
#include <navBar.hpp>

int main(int argc, char* argv[]) {
    std::cout << project_name << std::endl;
    std::cout << project_version << std::endl;
    std::cout << "C++ Standard: " << __cplusplus << std::endl;

    constexpr int WIDTH = 800;
    constexpr int HEIGHT = 600;

    Window window(WIDTH, HEIGHT, project_name);
    NavBar navBar;

    do {
        BeginDrawing();
            ClearBackground(DARKGRAY);

            rlImGuiBegin();
                navBar.draw();
            rlImGuiEnd();

        EndDrawing();
    } while(!WindowShouldClose());

    return 0;
}
