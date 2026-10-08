#pragma once

#include <navBar.hpp>

class AppLayout {
    public:
        AppLayout() = default;
        ~AppLayout() = default;

        void draw();
    private:
        NavBar navBar;
};
