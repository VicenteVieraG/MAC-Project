#include <window.hpp>

#include <stdexcept>
#include <string>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imguiConfig.hpp>
#include <implot.h>

namespace {
    std::runtime_error sdlError(const char* operation) {
        return std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }
}

Window::Window(int width, int height, std::string_view title) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        const auto error = sdlError("SDL_Init");
        SDL_Quit();
        throw error;
    }

    try {
        const std::string windowTitle(title);
        if (!SDL_CreateWindowAndRenderer(windowTitle.c_str(), width, height,
                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                &window, &renderer)) {
            throw sdlError("SDL_CreateWindowAndRenderer");
        }
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        if (!SDL_SetRenderVSync(renderer, 1)) {
            SDL_Log("VSync unavailable; using the 60 FPS frame limit: %s", SDL_GetError());
        }
        addIcon();

        IMGUI_CHECKVERSION();
        imguiContext = ImGui::CreateContext();
        imguiConfig::setup();

        platformInitialized = ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
        if (!platformInitialized) {
            throw sdlError("ImGui_ImplSDL3_InitForSDLRenderer");
        }
        rendererInitialized = ImGui_ImplSDLRenderer3_Init(renderer);
        if (!rendererInitialized) {
            throw sdlError("ImGui_ImplSDLRenderer3_Init");
        }
        implotContext = ImPlot::CreateContext();
    } catch (...) {
        shutdown();
        throw;
    }
}

Window::~Window() {
    shutdown();
}

void Window::shutdown() noexcept {
    if (implotContext) {
        ImPlot::DestroyContext(implotContext);
    }
    if (rendererInitialized) {
        ImGui_ImplSDLRenderer3_Shutdown();
    }
    if (platformInitialized) {
        ImGui_ImplSDL3_Shutdown();
    }
    if (imguiContext) {
        ImGui::DestroyContext(imguiContext);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Window::addIcon() const {
    SDL_Surface* icon = SDL_LoadPNG("assets/images/XtremeDevs.png");
    if (icon) {
        if (!SDL_SetWindowIcon(window, icon)) {
            SDL_Log("Window icon could not be set: %s", SDL_GetError());
        }
        SDL_DestroySurface(icon);
    } else {
        SDL_Log("Window icon failed to load: %s", SDL_GetError());
    }
}

bool Window::beginFrame() {
    for (;;) {
        SDL_Event event;
        bool quit = false;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT
                || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
                    && event.window.windowID == SDL_GetWindowID(window))
                || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                quit = true;
            }
        }
        if (quit) {
            return false;
        }
        if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
            break;
        }
        SDL_Delay(10);
    }

    frameStart = SDL_GetTicksNS();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    return true;
}

void Window::endFrame() {
    ImGui::Render();
    const auto& io = ImGui::GetIO();
    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    if (!SDL_RenderClear(renderer)) {
        throw sdlError("SDL_RenderClear");
    }
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    if (!SDL_RenderPresent(renderer)) {
        throw sdlError("SDL_RenderPresent");
    }

    constexpr Uint64 frameDuration = SDL_NS_PER_SECOND / 60;
    const Uint64 elapsed = SDL_GetTicksNS() - frameStart;
    if (elapsed < frameDuration) {
        SDL_DelayNS(frameDuration - elapsed);
    }
}
