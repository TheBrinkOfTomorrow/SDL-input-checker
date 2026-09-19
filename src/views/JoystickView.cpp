#include "views/JoystickView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
const char* powerName(SDL_PowerState p) {
    switch (p) {
        case SDL_POWERSTATE_ON_BATTERY: return "battery";
        case SDL_POWERSTATE_NO_BATTERY: return "wired";
        case SDL_POWERSTATE_CHARGING: return "charging";
        case SDL_POWERSTATE_CHARGED: return "charged";
        default: return "power ?";
    }
}
const char* connName(SDL_JoystickConnectionState c) {
    switch (c) {
        case SDL_JOYSTICK_CONNECTION_WIRED: return "USB";
        case SDL_JOYSTICK_CONNECTION_WIRELESS: return "wireless";
        default: return "conn ?";
    }
}
}  // namespace

JoystickView::JoystickView(SDL_Joystick* js) : js_(js) {
    const char* name = SDL_GetJoystickName(js);
    title_ = name ? name : "Joystick";
    char guid[64];
    SDL_GUIDToString(SDL_GetJoystickGUID(js), guid, sizeof guid);
    guid_ = guid;
    char buf[160];
    std::snprintf(buf, sizeof buf, "%04x:%04x  %s  %d axes  %d buttons  %d hats  %d balls",
                  SDL_GetJoystickVendor(js), SDL_GetJoystickProduct(js),
                  connName(SDL_GetJoystickConnectionState(js)), SDL_GetNumJoystickAxes(js),
                  SDL_GetNumJoystickButtons(js), SDL_GetNumJoystickHats(js), SDL_GetNumJoystickBalls(js));
    info_ = buf;
}

void JoystickView::handleEvent(const SDL_Event&) {}

void JoystickView::update(float dt) {
    state_.sample(js_);
    axisPeak_.resize(state_.axes.size(), 0.0f);
    buttonGlow_.resize(state_.buttons.size(), 0.0f);
    for (size_t i = 0; i < state_.axes.size(); ++i) {
        const float v = std::abs(static_cast<float>(state_.axes[i]) / 32767.0f);
        axisPeak_[i] = std::max(v, axisPeak_[i] - dt * 0.5f);
    }
    for (size_t i = 0; i < state_.buttons.size(); ++i) {
        buttonGlow_[i] = state_.buttons[i] ? 1.0f : std::max(0.0f, buttonGlow_[i] - dt * 3.0f);
    }
}

void JoystickView::draw(Canvas& c, const SDL_FRect& a) {
    const float pad = 24.0f;
    float x = a.x + pad, y = a.y + pad;

    c.text(x, y, title_.c_str(), colors::text, 2.5f);
    y += 28;
    c.text(x, y, info_.c_str(), colors::dim, 1.5f);
    y += 18;
    char pw[64];
    std::snprintf(pw, sizeof pw, "%s%s", powerName(state_.power),
                  state_.batteryPercent >= 0 ? ("  " + std::to_string(state_.batteryPercent) + "%").c_str() : "");
    c.textf(x, y, colors::dim, 1.5f, "GUID %s   %s", guid_.c_str(), pw);
    y += 36;

    // ---- Axes: one horizontal bar each, filled from the centre.
    const float barW = std::min(420.0f, a.w - pad * 2 - 60);
    const float barH = 16.0f, rowH = 30.0f;
    c.text(x, y, "AXES", colors::dim, 1.5f);
    y += 20;
    if (state_.axes.empty()) { c.text(x, y, "none", colors::idle); y += rowH; }
    for (size_t i = 0; i < state_.axes.size(); ++i) {
        const float v = static_cast<float>(state_.axes[i]) / 32767.0f;
        const float bx = x + 40;
        c.textf(x, y + 2, colors::text, 1.5f, "%zu", i);
        c.fillRect(bx, y, barW, barH, colors::panel);
        const float mid = bx + barW / 2;
        if (v >= 0) c.fillRect(mid, y, v * barW / 2, barH, colors::accent);
        else        c.fillRect(mid + v * barW / 2, y, -v * barW / 2, barH, colors::accent);
        // Peak markers.
        c.fillRect(mid + axisPeak_[i] * barW / 2 - 1, y, 2, barH, colors::accent2);
        c.fillRect(mid - axisPeak_[i] * barW / 2 - 1, y, 2, barH, colors::accent2);
        c.line(mid, y - 2, mid, y + barH + 2, colors::outline);
        c.strokeRect(bx, y, barW, barH, colors::outline);
        c.textf(bx + barW + 10, y + 2, colors::text, 1.5f, "%6d  %+.3f", state_.axes[i], v);
        y += rowH;
    }
    y += 12;

    // ---- Buttons: grid of squares.
    c.text(x, y, "BUTTONS", colors::dim, 1.5f);
    y += 20;
    const float cell = 40.0f, gap = 8.0f;
    const int perRow = std::max(1, static_cast<int>((a.w - pad * 2) / (cell + gap)));
    if (state_.buttons.empty()) { c.text(x, y, "none", colors::idle); y += cell; }
    for (size_t i = 0; i < state_.buttons.size(); ++i) {
        const float bx = x + static_cast<float>(i % static_cast<size_t>(perRow)) * (cell + gap);
        const float by = y + static_cast<float>(i / static_cast<size_t>(perRow)) * (cell + gap);
        const float g = buttonGlow_[i];
        Color fill{static_cast<Uint8>(colors::idle.r + (colors::accent.r - colors::idle.r) * g),
                   static_cast<Uint8>(colors::idle.g + (colors::accent.g - colors::idle.g) * g),
                   static_cast<Uint8>(colors::idle.b + (colors::accent.b - colors::idle.b) * g)};
        c.fillRect(bx, by, cell, cell, fill);
        c.strokeRect(bx, by, cell, cell, state_.buttons[i] ? colors::text : colors::outline);
        char label[8];
        std::snprintf(label, sizeof label, "%zu", i);
        const float tw = Canvas::textWidth(label, 1.5f);
        c.text(bx + (cell - tw) / 2, by + cell / 2 - 6, label, state_.buttons[i] ? colors::bg : colors::text, 1.5f);
    }
    if (!state_.buttons.empty())
        y += static_cast<float>((state_.buttons.size() + static_cast<size_t>(perRow) - 1) / static_cast<size_t>(perRow)) * (cell + gap);
    y += 12;

    // ---- Hats: 3x3 indicator each.
    if (!state_.hats.empty()) {
        c.text(x, y, "HATS", colors::dim, 1.5f);
        y += 20;
        const float hc = 22.0f;
        for (size_t i = 0; i < state_.hats.size(); ++i) {
            const float hx = x + static_cast<float>(i) * (hc * 3 + 30);
            const Uint8 h = state_.hats[i];
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    Uint8 mask = 0;
                    if (dy < 0) mask |= SDL_HAT_UP;
                    if (dy > 0) mask |= SDL_HAT_DOWN;
                    if (dx < 0) mask |= SDL_HAT_LEFT;
                    if (dx > 0) mask |= SDL_HAT_RIGHT;
                    const bool on = (dx == 0 && dy == 0) ? h == SDL_HAT_CENTERED : (h != 0 && h == mask);
                    c.fillRect(hx + static_cast<float>(dx + 1) * hc, y + static_cast<float>(dy + 1) * hc, hc - 2, hc - 2,
                               on ? colors::accent : colors::panel);
                }
            }
            c.textf(hx, y + hc * 3 + 4, colors::text, 1.5f, "%zu: 0x%02x", i, h);
        }
        y += hc * 3 + 30;
    }

    // ---- Balls.
    if (!state_.balls.empty()) {
        c.text(x, y, "BALLS", colors::dim, 1.5f);
        y += 20;
        for (size_t i = 0; i < state_.balls.size(); ++i) {
            c.textf(x, y, colors::text, 1.5f, "%zu: dx %+d  dy %+d", i, state_.balls[i].x, state_.balls[i].y);
            y += 18;
        }
    }
}
