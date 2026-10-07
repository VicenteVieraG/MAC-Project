#include <imguiConfig.hpp>

#include <imgui.h>

namespace imguiConfig {
    void setup() {
        auto& style = ImGui::GetStyle();
        auto& io = ImGui::GetIO();

        // Start with complete defaults, then apply the project's dark theme.
        ImGui::StyleColorsDark(&style);

        // Shared palette: layered charcoal surfaces and a cool blue accent.
        const ImVec4 background     {0.07f, 0.09f, 0.12f, 1.00f};
        const ImVec4 surface        {0.10f, 0.12f, 0.16f, 1.00f};
        const ImVec4 surfaceRaised  {0.14f, 0.17f, 0.22f, 1.00f};
        const ImVec4 surfaceHovered {0.19f, 0.24f, 0.31f, 1.00f};
        const ImVec4 surfaceActive  {0.20f, 0.29f, 0.42f, 1.00f};
        const ImVec4 border         {0.24f, 0.28f, 0.35f, 1.00f};
        const ImVec4 text           {0.93f, 0.95f, 0.98f, 1.00f};
        const ImVec4 textMuted      {0.58f, 0.64f, 0.73f, 1.00f};
        const ImVec4 accent         {0.39f, 0.67f, 1.00f, 1.00f};
        const ImVec4 accentHovered  {0.57f, 0.78f, 1.00f, 1.00f};
        const ImVec4 accentActive   {0.28f, 0.55f, 0.88f, 1.00f};
        const ImVec4 transparent    {0.00f, 0.00f, 0.00f, 0.00f};

        // Layout: comfortable spacing for the 18 px body font.
        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.5f;
        style.WindowPadding = ImVec2(16.0f, 16.0f);
        style.FramePadding = ImVec2(12.0f, 6.0f);
        style.ItemSpacing = ImVec2(12.0f, 8.0f);
        style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
        style.CellPadding = ImVec2(12.0f, 8.0f);
        style.IndentSpacing = 24.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 12.0f;

        // Rounded surfaces with thin borders; controls use filled backgrounds.
        style.WindowRounding = 10.0f;
        style.ChildRounding = 8.0f;
        style.PopupRounding = 8.0f;
        style.FrameRounding = 6.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 6.0f;
        style.MenuItemRounding = 4.0f;
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;
        style.TabBarBorderSize = 1.0f;
        style.TabBarOverlineSize = 2.0f;
        style.SeparatorTextBorderSize = 1.0f;
        style.SeparatorTextPadding = ImVec2(12.0f, 6.0f);
        style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.5f);
        style.AntiAliasedLines = true;
        style.AntiAliasedFill = true;

        auto& colors = style.Colors;

        // Text, windows, and navigation surfaces.
        colors[ImGuiCol_Text] = text;
        colors[ImGuiCol_TextDisabled] = textMuted;
        colors[ImGuiCol_WindowBg] = background;
        colors[ImGuiCol_ChildBg] = surface;
        colors[ImGuiCol_PopupBg] = surface;
        colors[ImGuiCol_Border] = border;
        colors[ImGuiCol_BorderShadow] = transparent;
        colors[ImGuiCol_TitleBg] = background;
        colors[ImGuiCol_TitleBgActive] = surface;
        colors[ImGuiCol_TitleBgCollapsed] = background;
        colors[ImGuiCol_MenuBarBg] = surface;

        // Inputs and actions share the same idle, hover, and pressed states.
        colors[ImGuiCol_FrameBg] = surfaceRaised;
        colors[ImGuiCol_FrameBgHovered] = surfaceHovered;
        colors[ImGuiCol_FrameBgActive] = surfaceActive;
        colors[ImGuiCol_Button] = surfaceRaised;
        colors[ImGuiCol_ButtonHovered] = surfaceHovered;
        colors[ImGuiCol_ButtonActive] = surfaceActive;
        colors[ImGuiCol_Header] = surfaceActive;
        colors[ImGuiCol_HeaderHovered] = surfaceHovered;
        colors[ImGuiCol_HeaderActive] = surfaceActive;
        colors[ImGuiCol_CheckMark] = accent;
        colors[ImGuiCol_CheckboxSelectedBg] = surfaceActive;
        colors[ImGuiCol_SliderGrab] = accent;
        colors[ImGuiCol_SliderGrabActive] = accentHovered;
        colors[ImGuiCol_InputTextCursor] = accent;
        colors[ImGuiCol_TextLink] = accent;
        colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.30f);

        // Scrollbars, separators, and resize affordances stay quiet until used.
        colors[ImGuiCol_ScrollbarBg] = background;
        colors[ImGuiCol_ScrollbarGrab] = border;
        colors[ImGuiCol_ScrollbarGrabHovered] = textMuted;
        colors[ImGuiCol_ScrollbarGrabActive] = accent;
        colors[ImGuiCol_Separator] = border;
        colors[ImGuiCol_SeparatorHovered] = accent;
        colors[ImGuiCol_SeparatorActive] = accentActive;
        colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.15f);
        colors[ImGuiCol_ResizeGripHovered] = accent;
        colors[ImGuiCol_ResizeGripActive] = accentActive;

        // Tabs retain an accent indicator when selected.
        colors[ImGuiCol_Tab] = surface;
        colors[ImGuiCol_TabHovered] = surfaceHovered;
        colors[ImGuiCol_TabSelected] = surfaceActive;
        colors[ImGuiCol_TabSelectedOverline] = accent;
        colors[ImGuiCol_TabDimmed] = background;
        colors[ImGuiCol_TabDimmedSelected] = surfaceRaised;
        colors[ImGuiCol_TabDimmedSelectedOverline] = textMuted;

        // Data views use subtle row banding and the same accent palette.
        colors[ImGuiCol_TableHeaderBg] = surfaceRaised;
        colors[ImGuiCol_TableBorderStrong] = border;
        colors[ImGuiCol_TableBorderLight] = surfaceRaised;
        colors[ImGuiCol_TableRowBg] = transparent;
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(text.x, text.y, text.z, 0.035f);
        colors[ImGuiCol_PlotLines] = accent;
        colors[ImGuiCol_PlotLinesHovered] = accentHovered;
        colors[ImGuiCol_PlotHistogram] = accentActive;
        colors[ImGuiCol_PlotHistogramHovered] = accentHovered;
        colors[ImGuiCol_TreeLines] = border;

        // Focus and overlays remain visible against every surface.
        colors[ImGuiCol_DragDropTarget] = accent;
        colors[ImGuiCol_DragDropTargetBg] = ImVec4(accent.x, accent.y, accent.z, 0.12f);
        colors[ImGuiCol_UnsavedMarker] = accent;
        colors[ImGuiCol_NavCursor] = accent;
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(text.x, text.y, text.z, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(background.x, background.y, background.z, 0.65f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(background.x, background.y, background.z, 0.75f);

        // Typography.
        if(auto* font = io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans-Regular.ttf", 18.0f))
            io.FontDefault = font;
    }
}
