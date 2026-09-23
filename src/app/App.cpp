#include "app/App.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstring>

#include "views/GamepadView.h"
#include "views/JoystickView.h"
#include "views/KeyboardView.h"
#include "views/MouseView.h"

namespace {

ImVec4 toImVec4(Color c, float a = 1.0f) {
    return ImVec4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, a);
}

// Ties the ImGui sidebar to the same dark/orange/blue palette the canvas
// views draw with (see draw/Canvas.h), so the whole window reads as one app
// rather than "ImGui defaults next to a custom renderer".
void applyTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 6);

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = toImVec4(colors::bg);
    c[ImGuiCol_ChildBg] = toImVec4(colors::bg);
    c[ImGuiCol_FrameBg] = toImVec4(colors::panel);
    c[ImGuiCol_FrameBgHovered] = toImVec4(colors::panel, 0.9f);
    c[ImGuiCol_FrameBgActive] = toImVec4(colors::idle);
    c[ImGuiCol_Text] = toImVec4(colors::text);
    c[ImGuiCol_TextDisabled] = toImVec4(colors::dim);
    c[ImGuiCol_Separator] = toImVec4(colors::outline, 0.5f);
    c[ImGuiCol_Header] = toImVec4(colors::accent, 0.55f);
    c[ImGuiCol_HeaderHovered] = toImVec4(colors::accent, 0.75f);
    c[ImGuiCol_HeaderActive] = toImVec4(colors::accent, 0.9f);
    c[ImGuiCol_Button] = toImVec4(colors::panel);
    c[ImGuiCol_ButtonHovered] = toImVec4(colors::accent, 0.8f);
    c[ImGuiCol_ButtonActive] = toImVec4(colors::accent);
    c[ImGuiCol_CheckMark] = toImVec4(colors::accent);
    c[ImGuiCol_SliderGrab] = toImVec4(colors::accent2);
    c[ImGuiCol_SliderGrabActive] = toImVec4(colors::accent2, 0.8f);
    c[ImGuiCol_TitleBgActive] = toImVec4(colors::panel);
    c[ImGuiCol_ScrollbarBg] = toImVec4(colors::bg);
    c[ImGuiCol_ScrollbarGrab] = toImVec4(colors::idle);
    c[ImGuiCol_ScrollbarGrabHovered] = toImVec4(colors::outline);
}

}  // namespace

App::~App() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
}

bool App::init(int argc, char** argv) {
    SDL_SetAppMetadata("SDL Input Checker", "0.1.0", "dev.silvan.sdl-input-checker");

    // Without this SDL drops gamepad button presses and hat changes while the
    // window is unfocused (axes still update). An input checker should keep
    // showing input even when the user is looking at another window.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    int initW = kCanvasW, initH = kCanvasH;
    if (const char* wh = SDL_getenv("SIC_WINDOW_SIZE")) {  // dev aid, e.g. "900x550", for resize testing
        SDL_sscanf(wh, "%dx%d", &initW, &initH);
    }
    if (!SDL_CreateWindowAndRenderer("SDL Input Checker", initW, initH,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &window_, &renderer_)) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(renderer_, 1);
    SDL_SetWindowMinimumSize(window_, 800, 500);
    SDL_Log("Renderer: %s", SDL_GetRendererName(renderer_));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // no imgui.ini clutter next to the binary
    applyTheme();

    // ImGui works in window points. Content scale (e.g. 2.0 on a Linux desktop set
    // to 200%) enlarges the widgets; pixel density (2.0 on Retina) is applied as an
    // SDL render scale at draw time so the backend outputs crisp pixels.
    const float content = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(window_));
    ImGui::GetStyle().ScaleAllSizes(content);
    ImGui::GetStyle().FontScaleDpi = content;

    ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer3_Init(renderer_);

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--virtual") == 0) {
            virtualJs_ = std::make_unique<VirtualJoystick>();
            if (!virtualJs_->attach()) virtualJs_.reset();
        }
    }
    screenshotPath_ = SDL_getenv("SIC_SCREENSHOT");
    if (const char* f = SDL_getenv("SIC_SCREENSHOT_FRAME")) screenshotFrame_ = SDL_atoi(f);

    registry_.refresh();
    if (virtualJs_) registry_.selectJoystickId(virtualJs_->id());
    if (const char* sel = SDL_getenv("SIC_SELECT")) {
        // Dev aid: "keyboard", "mouse", "gamepad", or "joystick" selects the first
        // matching entry, so screenshots can be scripted without clicking the UI.
        for (const DeviceInfo& d : registry_.devices()) {
            const bool match = (SDL_strcmp(sel, "keyboard") == 0 && d.id.kind == DeviceKind::Keyboard) ||
                               (SDL_strcmp(sel, "mouse") == 0 && d.id.kind == DeviceKind::Mouse) ||
                               (SDL_strcmp(sel, "gamepad") == 0 && d.id.kind == DeviceKind::Gamepad) ||
                               (SDL_strcmp(sel, "joystick") == 0 && d.id.kind == DeviceKind::Joystick);
            if (match) { registry_.select(d.id); break; }
        }
    }
    lastTicks_ = SDL_GetTicksNS();
    return true;
}

SDL_AppResult App::handleEvent(const SDL_Event& e) {
    ImGui_ImplSDL3_ProcessEvent(&e);
    registry_.handleEvent(e);
    if (view_) view_->handleEvent(e);
    switch (e.type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
            if (e.key.key == SDLK_ESCAPE) return SDL_APP_SUCCESS;
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult App::iterate() {
    const Uint64 now = SDL_GetTicksNS();
    const float dt = static_cast<float>(now - lastTicks_) / 1e9f;
    lastTicks_ = now;
    dt_ = dt;
    time_ += dt;
    ++frame_;
    if (virtualJs_) virtualJs_->animate(time_);

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ui_.draw(registry_, quitRequested_);
    if (quitRequested_) return SDL_APP_SUCCESS;
    syncView();
    if (view_) view_->update(dt_);

    ImGui::Render();

    SDL_SetRenderDrawColor(renderer_, 24, 26, 30, 255);
    SDL_RenderClear(renderer_);
    drawCanvas();
    const float density = SDL_GetWindowPixelDensity(window_);
    SDL_SetRenderScale(renderer_, density, density);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
    SDL_SetRenderScale(renderer_, 1.0f, 1.0f);
    if (screenshotPath_ && frame_ == screenshotFrame_) {
        if (SDL_Surface* surf = SDL_RenderReadPixels(renderer_, nullptr)) {
            // PNG unless the path explicitly asks for .bmp.
            const size_t len = SDL_strlen(screenshotPath_);
            const bool bmp = len >= 4 && SDL_strcasecmp(screenshotPath_ + len - 4, ".bmp") == 0;
            const bool ok = bmp ? SDL_SaveBMP(surf, screenshotPath_) : SDL_SavePNG(surf, screenshotPath_);
            SDL_DestroySurface(surf);
            if (ok) SDL_Log("Saved screenshot to %s", screenshotPath_);
            else    SDL_Log("Failed to save screenshot to %s: %s", screenshotPath_, SDL_GetError());
        }
        SDL_RenderPresent(renderer_);
        return SDL_APP_SUCCESS;
    }
    SDL_RenderPresent(renderer_);
    return SDL_APP_CONTINUE;
}

void App::syncView() {
    const std::optional<DeviceId> sel = registry_.selected();
    if (sel == viewFor_) return;
    view_.reset();
    viewFor_ = sel;
    if (!sel) return;
    switch (sel->kind) {
        case DeviceKind::Gamepad:
            if (registry_.gamepad()) view_ = std::make_unique<GamepadView>(registry_.gamepad());
            break;
        case DeviceKind::Joystick:
            if (registry_.joystick()) view_ = std::make_unique<JoystickView>(registry_.joystick());
            break;
        case DeviceKind::Keyboard:
            view_ = std::make_unique<KeyboardView>(window_);
            break;
        case DeviceKind::Mouse:
            view_ = std::make_unique<MouseView>(window_);
            break;
    }
}

void App::drawCanvas() {
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(renderer_, &w, &h);
    const float scale = SDL_GetWindowDisplayScale(window_);
    // Area right of the sidebar, in window points.
    const SDL_FRect area{static_cast<float>(kSidebarW), 0.0f,
                         static_cast<float>(w) / scale - kSidebarW, static_cast<float>(h) / scale};
    Canvas canvas(renderer_, scale);
    canvas.fillRect(area.x, area.y, area.w, area.h, colors::bg);

    if (view_) {
        view_->draw(canvas, area);
        return;
    }
    const DeviceInfo* sel = registry_.selectedInfo();
    if (sel) {
        canvas.textf(area.x + 24, 24, colors::text, 2.0f, "%s: %s", deviceKindName(sel->id.kind), sel->name.c_str());
        canvas.text(area.x + 24, 52, "Visualiser for this device kind arrives in a later milestone.", colors::dim);
    } else {
        canvas.text(area.x + 24, 24, "Select a device in the sidebar to visualise it.", colors::dim, 2.0f);
    }
}
