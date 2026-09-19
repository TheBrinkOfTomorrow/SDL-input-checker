#include "views/GamepadView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const char* buttonLabel(SDL_Gamepad* gp, SDL_GamepadButton b) {
    switch (SDL_GetGamepadButtonLabel(gp, b)) {
        case SDL_GAMEPAD_BUTTON_LABEL_A: return "A";
        case SDL_GAMEPAD_BUTTON_LABEL_B: return "B";
        case SDL_GAMEPAD_BUTTON_LABEL_X: return "X";
        case SDL_GAMEPAD_BUTTON_LABEL_Y: return "Y";
        case SDL_GAMEPAD_BUTTON_LABEL_CROSS: return "X";
        case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE: return "O";
        case SDL_GAMEPAD_BUTTON_LABEL_SQUARE: return "#";
        case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE: return "^";
        default: return "?";
    }
}

const char* powerName(SDL_PowerState p) {
    switch (p) {
        case SDL_POWERSTATE_ON_BATTERY: return "on battery";
        case SDL_POWERSTATE_NO_BATTERY: return "wired";
        case SDL_POWERSTATE_CHARGING: return "charging";
        case SDL_POWERSTATE_CHARGED: return "charged";
        default: return "power unknown";
    }
}

const char* connName(SDL_JoystickConnectionState c) {
    switch (c) {
        case SDL_JOYSTICK_CONNECTION_WIRED: return "USB";
        case SDL_JOYSTICK_CONNECTION_WIRELESS: return "wireless";
        default: return "connection unknown";
    }
}

Color lerp(Color a, Color b, float t) {
    return {static_cast<Uint8>(a.r + (b.r - a.r) * t), static_cast<Uint8>(a.g + (b.g - a.g) * t),
            static_cast<Uint8>(a.b + (b.b - a.b) * t)};
}

}  // namespace

GamepadView::GamepadView(SDL_Gamepad* gp) : gp_(gp) {
    const char* name = SDL_GetGamepadName(gp);
    title_ = name ? name : "Gamepad";
    char buf[192];
    const int player = SDL_GetGamepadPlayerIndex(gp);
    std::snprintf(buf, sizeof buf, "%04x:%04x  %s  %s%s%s", SDL_GetGamepadVendor(gp), SDL_GetGamepadProduct(gp),
                  connName(SDL_GetGamepadConnectionState(gp)), powerName(SDL_GetGamepadPowerInfo(gp, nullptr)),
                  player >= 0 ? "  player " : "", player >= 0 ? std::to_string(player + 1).c_str() : "");
    subtitle_ = buf;
    if (char* m = SDL_GetGamepadMapping(gp)) {
        mapping_ = m;
        SDL_free(m);
    }
    state_.sampleCapabilities(gp);
    buttonGlow_.assign(SDL_GAMEPAD_BUTTON_COUNT, 0.0f);
}

void GamepadView::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_EVENT_MOUSE_BUTTON_DOWN || e.button.button != SDL_BUTTON_LEFT) return;
    const SDL_FPoint p{e.button.x, e.button.y};
    for (const HitRect& hr : hitRects_) {
        if (p.x >= hr.r.x && p.x <= hr.r.x + hr.r.w && p.y >= hr.r.y && p.y <= hr.r.y + hr.r.h) {
            fireAction(hr.action);
            break;
        }
    }
}

void GamepadView::fireAction(int action) {
    switch (action) {
        case 0:
            SDL_RumbleGamepad(gp_, 0xFFFF, 0xFFFF, 300);
            rumbleFlashTtl_ = 0.3f;
            break;
        case 1:
            SDL_RumbleGamepadTriggers(gp_, 0xFFFF, 0xFFFF, 300);
            rumbleFlashTtl_ = 0.3f;
            break;
        case 2: {
            static constexpr Color kCycle[] = {{255, 60, 60}, {60, 255, 90}, {70, 140, 255}, {255, 255, 255}};
            ledIndex_ = (ledIndex_ + 1) % 4;
            const Color c = kCycle[ledIndex_];
            SDL_SetGamepadLED(gp_, c.r, c.g, c.b);
            break;
        }
        default: break;
    }
}

void GamepadView::update(float dt) {
    state_.sample(gp_);
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        float& g = buttonGlow_[static_cast<size_t>(b)];
        g = state_.buttons[static_cast<size_t>(b)] ? 1.0f : std::max(0.0f, g - dt * 4.0f);
    }
    rumbleFlashTtl_ = std::max(0.0f, rumbleFlashTtl_ - dt);
}

void GamepadView::drawStick(Canvas& c, float cx, float cy, float radius, float x, float y, bool pressed, float glow) {
    c.strokeCircle(cx, cy, radius, colors::outline);
    c.strokeCircle(cx, cy, radius * 0.18f, colors::idle);  // deadzone reference ring
    const float dx = cx + x * (radius - 10.0f);
    const float dy = cy + y * (radius - 10.0f);
    c.line(cx, cy, dx, dy, colors::outline);
    c.fillCircle(dx, dy, 10.0f, pressed ? lerp(colors::accent2, colors::text, 0.0f) : lerp(colors::accent2, colors::idle, 1.0f - std::max(glow, std::abs(x) + std::abs(y) > 0.02f ? 0.6f : 0.0f)));
    c.strokeCircle(dx, dy, 10.0f, colors::text);
}

void GamepadView::drawTrigger(Canvas& c, float x, float y, float w, float h, float value, const char* label) {
    c.fillRect(x, y, w, h, colors::panel);
    c.fillRect(x, y + h * (1.0f - value), w, h * value, colors::accent);
    c.strokeRect(x, y, w, h, colors::outline);
    const float tw = Canvas::textWidth(label, 1.2f);
    c.text(x + (w - tw) / 2, y + h + 4, label, colors::dim, 1.2f);
}

void GamepadView::drawFaceButton(Canvas& c, float cx, float cy, float radius, SDL_GamepadButton btn, float glow) {
    if (!SDL_GamepadHasButton(gp_, btn)) return;
    const Color fill = lerp(colors::idle, colors::accent, glow);
    c.fillCircle(cx, cy, radius, fill);
    c.strokeCircle(cx, cy, radius, colors::outline);
    const char* label = buttonLabel(gp_, btn);
    const float tw = Canvas::textWidth(label, 1.6f);
    c.text(cx - tw / 2, cy - 7, label, glow > 0.5f ? colors::bg : colors::text, 1.6f);
}

void GamepadView::drawSmallButton(Canvas& c, float cx, float cy, float radius, const char* label, bool pressed) {
    c.fillCircle(cx, cy, radius, pressed ? colors::accent : colors::panel);
    c.strokeCircle(cx, cy, radius, colors::outline);
    const float tw = Canvas::textWidth(label, 1.0f);
    c.text(cx - tw / 2, cy - 5, label, pressed ? colors::bg : colors::dim, 1.0f);
}

void GamepadView::draw(Canvas& c, const SDL_FRect& a) {
    hitRects_.clear();
    const float pad = 24.0f;
    float x = a.x + pad, y = a.y + pad;

    c.text(x, y, title_.c_str(), colors::text, 2.5f);
    y += 28;
    c.text(x, y, subtitle_.c_str(), colors::dim, 1.5f);
    y += 18;
    if (state_.batteryPercent >= 0) c.textf(x, y, colors::dim, 1.5f, "battery %d%%", state_.batteryPercent);
    y += 24;

    const float bodyTop = y + 20;
    const float bodyCx = a.x + a.w * 0.5f;

    // ---- Triggers + shoulders across the top.
    const float trigW = 44, trigH = 60;
    drawTrigger(c, bodyCx - 220, bodyTop, trigW, trigH, state_.axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER], "LT");
    drawTrigger(c, bodyCx + 220 - trigW, bodyTop, trigW, trigH, state_.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER], "RT");
    const float shoulderY = bodyTop + trigH + 14;
    const bool lb = state_.buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER], rb = state_.buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER];
    c.fillRect(bodyCx - 220, shoulderY, trigW, 20, lb ? colors::accent : colors::panel);
    c.strokeRect(bodyCx - 220, shoulderY, trigW, 20, colors::outline);
    c.text(bodyCx - 220 + 6, shoulderY + 3, "LB", lb ? colors::bg : colors::dim, 1.1f);
    c.fillRect(bodyCx + 220 - trigW, shoulderY, trigW, 20, rb ? colors::accent : colors::panel);
    c.strokeRect(bodyCx + 220 - trigW, shoulderY, trigW, 20, colors::outline);
    c.text(bodyCx + 220 - trigW + 6, shoulderY + 3, "RB", rb ? colors::bg : colors::dim, 1.1f);

    const float rowY = shoulderY + 60;

    // ---- Back / Guide / Start in the middle.
    drawSmallButton(c, bodyCx - 40, rowY, 14, "<", state_.buttons[SDL_GAMEPAD_BUTTON_BACK]);
    if (SDL_GamepadHasButton(gp_, SDL_GAMEPAD_BUTTON_GUIDE))
        drawSmallButton(c, bodyCx, rowY, 16, "G", state_.buttons[SDL_GAMEPAD_BUTTON_GUIDE]);
    drawSmallButton(c, bodyCx + 40, rowY, 14, ">", state_.buttons[SDL_GAMEPAD_BUTTON_START]);

    // ---- Left stick + D-pad, right stick + face buttons.
    const float stickY = rowY + 90;
    drawStick(c, bodyCx - 180, stickY, 46, state_.axes[SDL_GAMEPAD_AXIS_LEFTX], state_.axes[SDL_GAMEPAD_AXIS_LEFTY],
              state_.buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK], buttonGlow_[SDL_GAMEPAD_BUTTON_LEFT_STICK]);
    drawStick(c, bodyCx + 180, stickY, 46, state_.axes[SDL_GAMEPAD_AXIS_RIGHTX], state_.axes[SDL_GAMEPAD_AXIS_RIGHTY],
              state_.buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK], buttonGlow_[SDL_GAMEPAD_BUTTON_RIGHT_STICK]);

    // D-pad: cross of 3 squares wide, centred below-left.
    const float dpadCx = bodyCx - 60, dpadCy = stickY + 90, dc = 24;
    struct { SDL_GamepadButton b; int dx, dy; const char* l; } dpad[] = {
        {SDL_GAMEPAD_BUTTON_DPAD_UP, 0, -1, "^"}, {SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, 1, "v"},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, -1, 0, "<"}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 1, 0, ">"}};
    for (auto& d : dpad) {
        const float bx = dpadCx + static_cast<float>(d.dx) * dc, by = dpadCy + static_cast<float>(d.dy) * dc;
        const bool on = state_.buttons[d.b];
        c.fillRect(bx - dc / 2, by - dc / 2, dc, dc, on ? colors::accent : colors::panel);
        c.strokeRect(bx - dc / 2, by - dc / 2, dc, dc, colors::outline);
        c.text(bx - 4, by - 6, d.l, on ? colors::bg : colors::dim, 1.2f);
    }
    c.fillRect(dpadCx - dc / 2, dpadCy - dc / 2, dc, dc, colors::panel);
    c.strokeRect(dpadCx - dc / 2, dpadCy - dc / 2, dc, dc, colors::outline);

    // Face buttons: diamond, mirrored position of the D-pad.
    const float faceCx = bodyCx + 60, faceCy = dpadCy, fc = 30;
    drawFaceButton(c, faceCx, faceCy - fc, 17, SDL_GAMEPAD_BUTTON_NORTH, buttonGlow_[SDL_GAMEPAD_BUTTON_NORTH]);
    drawFaceButton(c, faceCx, faceCy + fc, 17, SDL_GAMEPAD_BUTTON_SOUTH, buttonGlow_[SDL_GAMEPAD_BUTTON_SOUTH]);
    drawFaceButton(c, faceCx - fc, faceCy, 17, SDL_GAMEPAD_BUTTON_WEST, buttonGlow_[SDL_GAMEPAD_BUTTON_WEST]);
    drawFaceButton(c, faceCx + fc, faceCy, 17, SDL_GAMEPAD_BUTTON_EAST, buttonGlow_[SDL_GAMEPAD_BUTTON_EAST]);

    // Misc / paddle buttons, if present: a row of small squares below.
    float miscY = dpadCy + 60;
    struct { SDL_GamepadButton b; const char* l; } miscs[] = {
        {SDL_GAMEPAD_BUTTON_MISC1, "M1"}, {SDL_GAMEPAD_BUTTON_LEFT_PADDLE1, "LP1"},
        {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1, "RP1"}, {SDL_GAMEPAD_BUTTON_LEFT_PADDLE2, "LP2"},
        {SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2, "RP2"}, {SDL_GAMEPAD_BUTTON_TOUCHPAD, "PAD"}};
    float miscX = a.x + pad;
    bool anyMisc = false;
    for (auto& m : miscs) {
        if (!SDL_GamepadHasButton(gp_, m.b)) continue;
        anyMisc = true;
        const bool on = state_.buttons[m.b];
        c.fillRect(miscX, miscY, 44, 24, on ? colors::accent : colors::panel);
        c.strokeRect(miscX, miscY, 44, 24, colors::outline);
        c.text(miscX + 4, miscY + 4, m.l, on ? colors::bg : colors::dim, 1.1f);
        miscX += 52;
    }
    if (anyMisc) miscY += 34;

    // ---- Touchpad fingers.
    float py = miscY + 10;
    for (size_t t = 0; t < state_.touchpads.size(); ++t) {
        const float pw = 220, ph = 120;
        const float px = a.x + pad;
        c.text(px, py, "TOUCHPAD", colors::dim, 1.3f);
        py += 18;
        c.fillRect(px, py, pw, ph, colors::panel);
        c.strokeRect(px, py, pw, ph, colors::outline);
        for (const GamepadTouch& f : state_.touchpads[t]) {
            if (!f.down) continue;
            c.fillCircle(px + f.x * pw, py + f.y * ph, 6 + f.pressure * 6, colors::accent2);
        }
        py += ph + 16;
    }

    // ---- Gyro / accelerometer bars.
    float sensorBottom = 0.0f;
    if (state_.hasGyro || state_.hasAccel) {
        const float sx = bodyCx + 130, sw = 160, sh = 10;
        float sy = shoulderY + 28;  // clear the LT/RT value labels printed just above
        auto bar = [&](const char* label, float v, float range) {
            c.text(sx, sy, label, colors::dim, 1.1f);
            const float bx = sx + 40;
            c.fillRect(bx, sy, sw, sh, colors::panel);
            const float t = std::clamp(v / range, -1.0f, 1.0f);
            const float mid = bx + sw / 2;
            if (t >= 0) c.fillRect(mid, sy, t * sw / 2, sh, colors::good);
            else        c.fillRect(mid + t * sw / 2, sy, -t * sw / 2, sh, colors::good);
            c.strokeRect(bx, sy, sw, sh, colors::outline);
            sy += sh + 6;
        };
        if (state_.hasGyro) { bar("gx", state_.gyro[0], 10.0f); bar("gy", state_.gyro[1], 10.0f); bar("gz", state_.gyro[2], 10.0f); }
        if (state_.hasAccel) { bar("ax", state_.accel[0], 20.0f); bar("ay", state_.accel[1], 20.0f); bar("az", state_.accel[2], 20.0f); }
        sensorBottom = sy;
    }

    // ---- Rumble / LED test controls: flow below whichever column (touchpad
    // or sensors) runs deeper, so this never collides with the D-pad/face
    // buttons above on a short window, and never leaves a large empty gap on
    // a tall one.
    float ty = std::max({py + 10.0f, sensorBottom + 10.0f, dpadCy + 60.0f});
    const float tx = a.x + pad;
    auto testButton = [&](float bx, const char* label, int action, bool active) {
        const SDL_FRect r{bx, ty, 130, 32};
        c.fillRect(r.x, r.y, r.w, r.h, active ? colors::accent : colors::panel);
        c.strokeRect(r.x, r.y, r.w, r.h, colors::outline);
        const float tw = Canvas::textWidth(label, 1.3f);
        c.text(r.x + (r.w - tw) / 2, r.y + 9, label, active ? colors::bg : colors::text, 1.3f);
        hitRects_.push_back({r, action});
    };
    if (state_.hasRumble) testButton(tx, "Test rumble", 0, rumbleFlashTtl_ > 0.0f);
    if (state_.hasRumbleTriggers) testButton(tx + 140, "Test trigger rumble", 1, rumbleFlashTtl_ > 0.0f);
    if (state_.hasMonoLed || state_.hasRgbLed || state_.hasPlayerLed) testButton(tx + 280, "Cycle LED", 2, false);

    if (!mapping_.empty()) {
        std::string m = mapping_.substr(0, std::min<size_t>(mapping_.size(), 90));
        c.text(tx, ty + 44, ("mapping: " + m + (mapping_.size() > 90 ? "..." : "")).c_str(), colors::dim, 1.0f);
    }
}
