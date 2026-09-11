#pragma once

enum class Screen {
    Home,
    Bayes
};

class NavBar {
    public: NavBar();
    public: void draw();
    public: Screen getCurrentScreen() const noexcept;
    private: Screen currentScreen;
};
