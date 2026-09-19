#include "views/MouseView.h"
#include <algorithm>

#include <cmath>

MouseView::MouseView(SDL_Window* window) : window_(window) {}

MouseView::~MouseView() {
    if (relativeMode_) SDL_SetWindowRelativeMouseMode(window_, false);
}

void MouseView::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_EVENT_MOUSE_MOTION) {
        state_.onMotion(e.motion);
    } else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        state_.onWheel(e.wheel);
    } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
        const SDL_FPoint p{e.button.x, e.button.y};
        if (p.x >= toggleRect_.x && p.x <= toggleRect_.x + toggleRect_.w && p.y >= toggleRect_.y &&
            p.y <= toggleRect_.y + toggleRect_.h) {
            relativeMode_ = !relativeMode_;
            SDL_SetWindowRelativeMouseMode(window_, relativeMode_);
        }
    }
}

void MouseView::update(float dt) {
    state_.sample(window_, dt);
}

void MouseView::draw(Canvas& c, const SDL_FRect& a) {
    const float pad = 24.0f;
    float x = a.x + pad, y = a.y + pad;

    c.text(x, y, "Mouse", colors::text, 2.5f);
    y += 30;
    c.textf(x, y, colors::dim, 1.4f, "position %.0f, %.0f   relative mode: %s", state_.x, state_.y,
            relativeMode_ ? "on (deltas only, cursor hidden)" : "off");
    y += 30;

    // ---- Silhouette: body + left/right/middle buttons + two side buttons.
    const float bodyW = 140, bodyH = 220, bx = x, by = y;
    const bool l = state_.buttons & SDL_BUTTON_LMASK, r = state_.buttons & SDL_BUTTON_RMASK,
               m = state_.buttons & SDL_BUTTON_MMASK, x1 = state_.buttons & SDL_BUTTON_X1MASK,
               x2 = state_.buttons & SDL_BUTTON_X2MASK;
    c.fillRect(bx, by, bodyW, bodyH, colors::panel);
    c.strokeRect(bx, by, bodyW, bodyH, colors::outline);
    c.fillRect(bx, by, bodyW / 2 - 2, 90, l ? colors::accent : colors::idle);
    c.fillRect(bx + bodyW / 2 + 2, by, bodyW / 2 - 2, 90, r ? colors::accent : colors::idle);
    c.strokeRect(bx, by, bodyW / 2 - 2, 90, colors::outline);
    c.strokeRect(bx + bodyW / 2 + 2, by, bodyW / 2 - 2, 90, colors::outline);
    c.text(bx + 6, by + 4, "L", l ? colors::bg : colors::dim, 1.2f);
    c.text(bx + bodyW - 16, by + 4, "R", r ? colors::bg : colors::dim, 1.2f);
    c.fillRect(bx + bodyW / 2 - 12, by + 6, 24, 40, m ? colors::accent : colors::bg);
    c.strokeRect(bx + bodyW / 2 - 12, by + 6, 24, 40, colors::outline);
    c.fillCircle(bx - 14, by + 130, 12, x1 ? colors::accent : colors::idle);
    c.strokeCircle(bx - 14, by + 130, 12, colors::outline);
    c.text(bx - 20, by + 122, "1", x1 ? colors::bg : colors::dim, 1.0f);
    c.fillCircle(bx - 14, by + 160, 12, x2 ? colors::accent : colors::idle);
    c.strokeCircle(bx - 14, by + 160, 12, colors::outline);
    c.text(bx - 20, by + 152, "2", x2 ? colors::bg : colors::dim, 1.0f);

    // ---- Wheel meter to the right of the body.
    const float wx = bx + bodyW + 50, wy = by, wh = bodyH;
    c.fillRect(wx, wy, 20, wh, colors::panel);
    c.strokeRect(wx, wy, 20, wh, colors::outline);
    const float wv = std::clamp(state_.wheelY / 5.0f, -1.0f, 1.0f);
    const float wmid = wy + wh / 2;
    if (wv >= 0) c.fillRect(wx, wmid - wv * wh / 2, 20, wv * wh / 2, colors::accent2);
    else         c.fillRect(wx, wmid, 20, -wv * wh / 2, colors::accent2);
    c.line(wx - 4, wmid, wx + 24, wmid, colors::outline);
    c.text(wx - 6, wy + wh + 6, "wheel", colors::dim, 1.1f);

    // ---- XY motion pad with trail, to the right of the wheel.
    const float px = wx + 70, py = by, pw = 260, ph = bodyH;
    c.fillRect(px, py, pw, ph, colors::panel);
    c.strokeRect(px, py, pw, ph, colors::outline);
    c.line(px, py + ph / 2, px + pw, py + ph / 2, colors::idle);
    c.line(px + pw / 2, py, px + pw / 2, py + ph, colors::idle);
    if (!state_.trail.empty()) {
        const SDL_FPoint ref = state_.trail.back();
        for (size_t i = 0; i < state_.trail.size(); ++i) {
            const SDL_FPoint& p = state_.trail[i];
            const float t = static_cast<float>(i) / static_cast<float>(state_.trail.size());
            const float tx = px + pw / 2 + std::clamp((p.x - ref.x), -pw / 2 + 4, pw / 2 - 4);
            const float ty = py + ph / 2 + std::clamp((p.y - ref.y), -ph / 2 + 4, ph / 2 - 4);
            c.fillCircle(tx, ty, 2.0f + t * 2.0f, colors::lerp(colors::idle, colors::accent2, t));
        }
    }
    c.text(px, py + ph + 6, "motion trail (recent positions, latest centred)", colors::dim, 1.1f);

    // ---- Numeric readout + relative-mode toggle.
    float ty = by + bodyH + 40;
    c.textf(x, ty, colors::text, 1.3f, "buttons: %s%s%s%s%s", l ? "L " : "", r ? "R " : "", m ? "M " : "",
            x1 ? "X1 " : "", x2 ? "X2 " : "");
    ty += 22;
    c.textf(x, ty, colors::text, 1.3f, "last delta: %+.0f, %+.0f", state_.relX, state_.relY);
    ty += 22;
    c.textf(x, ty, colors::text, 1.3f, "wheel raw: %+.2f, %+.2f", state_.wheelX, state_.wheelY);
    ty += 30;

    toggleRect_ = {x, ty, 240, 32};
    c.fillRect(toggleRect_.x, toggleRect_.y, toggleRect_.w, toggleRect_.h, relativeMode_ ? colors::accent : colors::panel);
    c.strokeRect(toggleRect_.x, toggleRect_.y, toggleRect_.w, toggleRect_.h, colors::outline);
    const char* label = relativeMode_ ? "Relative mode: ON (click to exit)" : "Enable relative mouse mode";
    c.text(toggleRect_.x + 10, toggleRect_.y + 9, label, relativeMode_ ? colors::bg : colors::text, 1.2f);

    state_.endFrame();
}
