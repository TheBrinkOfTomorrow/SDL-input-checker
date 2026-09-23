#pragma once
#include <SDL3/SDL.h>

#include <memory>
#include <optional>

#include "app/Ui.h"
#include "devices/DeviceRegistry.h"
#include "devices/VirtualJoystick.h"
#include "views/View.h"

class App {
public:
    static constexpr int kCanvasW = 1280;
    static constexpr int kCanvasH = 800;
    static constexpr int kSidebarW = 280;

    App() = default;
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool init(int argc, char** argv);
    SDL_AppResult handleEvent(const SDL_Event& e);
    SDL_AppResult iterate();

private:
    void drawCanvas();
    void syncView();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    DeviceRegistry registry_;
    Ui ui_;
    std::unique_ptr<View> view_;
    std::optional<DeviceId> viewFor_;
    float dt_ = 0.0f;
    float time_ = 0.0f;
    int frame_ = 0;
    std::unique_ptr<VirtualJoystick> virtualJs_;
    const char* screenshotPath_ = nullptr;  // SIC_SCREENSHOT env: save a frame as PNG (or BMP for .bmp) and quit
    int screenshotFrame_ = 30;               // SIC_SCREENSHOT_FRAME env
    Uint64 lastTicks_ = 0;
    bool quitRequested_ = false;
};
