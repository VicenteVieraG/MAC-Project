#include <imguiConfig.hpp>

#include <imgui.h>

namespace imguiConfig {
    void setup(){
        // Configuration Objects
        auto& style = ImGui::GetStyle();
        auto& io = ImGui::GetIO();

        // Base Theme
        ImGui::StyleColorsDark();

        // Fonts
        if(auto* font = io.Fonts->
            AddFontFromFileTTF("assets/fonts/NotoSans-Regular.ttf", 18.0f))
                io.FontDefault = font;
    }
};