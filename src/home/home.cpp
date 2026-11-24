#include <home.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <imgui.h>
#include <rlgl.h>

namespace {
std::string assetPath(const char* path) {
    if (FileExists(path)) return path;
    return (std::filesystem::path(GetApplicationDirectory()) / path).string();
}

void drawPlatform() {
    constexpr int slices = 144;
    DrawCylinder({0, -0.40f, 0}, 5.9f, 5.65f, .36f, slices, {9, 22, 41, 255});
    DrawCylinder({0, -0.08f, 0}, 5.80f, 5.82f, .055f, slices, {39, 183, 211, 255});
    DrawCylinder({0, -0.035f, 0}, 5.68f, 5.80f, .11f, slices, {20, 43, 63, 255});
    DrawCylinder({0, .078f, 0}, 5.50f, 5.50f, .014f, slices, {13, 31, 51, 255});

    for (int i = 0; i < slices; ++i) {
        const float angle = 2 * PI * i / slices;
        const float x = std::cos(angle), z = std::sin(angle);
        const bool major = i % 6 == 0;
        const float inner = major ? 5.16f : 5.36f;
        DrawLine3D({inner*x, .10f, inner*z}, {5.43f*x, .10f, 5.43f*z},
                   major ? Color{86, 192, 208, 255} : Color{44, 87, 108, 255});
    }
}
}

Home::Home() {
    logo = LoadModel(assetPath("assets/models/XtremeDevs.glb").c_str());
    sky = LoadShader(nullptr, assetPath("assets/shaders/home-sky.fs").c_str());
    resolutionLocation = GetShaderLocation(sky, "resolution");
    driftLocation = GetShaderLocation(sky, "drift");
}

Home::~Home() {
    if (canvas.id) UnloadRenderTexture(canvas);
    // Raylib leaves material textures owned by the caller.
    if (IsModelValid(logo)) {
        const auto albedo = logo.materials[logo.meshMaterial[0]].maps[MATERIAL_MAP_ALBEDO].texture;
        if (albedo.id && albedo.id != rlGetTextureIdDefault()) UnloadTexture(albedo);
    }
    UnloadModel(logo);
    UnloadShader(sky);
}

void Home::draw() {
    const ImVec2 size = ImGui::GetContentRegionAvail();
    const int width = static_cast<int>(size.x), height = static_cast<int>(size.y);
    if (width < 1 || height < 1) return;
    // Supersample small windows to keep thin circuit traces and platform markings clean.
    const float scale = std::clamp(1920.f / std::max(width, height), 1.f, 2.f);
    const int renderWidth = static_cast<int>(width * scale);
    const int renderHeight = static_cast<int>(height * scale);
    if (canvas.texture.width != renderWidth || canvas.texture.height != renderHeight) {
        if (canvas.id) UnloadRenderTexture(canvas);
        canvas = LoadRenderTexture(renderWidth, renderHeight);
        if (canvas.id) SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
    }
    if (!IsRenderTextureValid(canvas)) return;

    const float drift = static_cast<float>(std::sin(GetTime() * .24));
    const Vector2 resolution{static_cast<float>(renderWidth), static_cast<float>(renderHeight)};
    const float aspect = resolution.x / resolution.y;
    Camera3D camera{};
    camera.position = {4.4f + 1.5f*drift, 7.5f, 27.f};
    camera.target = {0.f, 3.8f, 0.f};
    camera.up = {0.f, 1.f, 0.f};
    camera.fovy = std::max(10.8f, 12.7f / aspect);
    camera.projection = CAMERA_ORTHOGRAPHIC;

    BeginTextureMode(canvas);
    ClearBackground({8, 18, 35, 255});
    SetShaderValue(sky, resolutionLocation, &resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(sky, driftLocation, &drift, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(sky);
    DrawRectangle(0, 0, renderWidth, renderHeight, WHITE);
    EndShaderMode();

    BeginMode3D(camera);
    drawPlatform();
    DrawModel(logo, {0.f, .53f + .075f*drift, 0.f}, 1.f, WHITE);
    EndMode3D();
    EndTextureMode();
    ImGui::Image(static_cast<ImTextureID>(canvas.texture.id), size, {0, 1}, {1, 0});
}
