#pragma once

enum class Screen {
    Home,
    Bayes,
    Search,
    Coloring
};

class NavBar {
    public: NavBar();
    public: void draw();
    public: Screen getCurrentScreen() const noexcept;
    private: Screen currentScreen;
};
