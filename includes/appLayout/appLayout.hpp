#pragma once

#include <navBar.hpp>
#include <home.hpp>
#include <bayes.hpp>
#include <search.hpp>

class AppLayout {
    public:
        AppLayout() = default;
        ~AppLayout() = default;

        void draw();
    private:
        NavBar navBar;
        Home home;
        Bayes bayes;
        Search search;
};
