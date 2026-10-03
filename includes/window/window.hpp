#pragma once

#include <string_view>

class Window {
    public:
        Window(int width, int height, std::string_view title);
        ~Window();

        void addIcon() const;
    private:
        int width;
        int height;
};