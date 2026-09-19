#include "views/KeyboardView.h"
#include <algorithm>

#include <cstdio>

#include "devices/KeyboardLayout.h"

KeyboardView::KeyboardView(SDL_Window* window) : window_(window) {
    SDL_StartTextInput(window_);  // enables SDL_EVENT_TEXT_INPUT for IME/dead-key testing
}

KeyboardView::~KeyboardView() { SDL_StopTextInput(window_); }

void KeyboardView::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
        state_.onKeyEvent(e.key);
    } else if (e.type == SDL_EVENT_TEXT_INPUT) {
        state_.onTextInput(e.text.text);
    }
}

void KeyboardView::update(float) { state_.sample(); }

void KeyboardView::draw(Canvas& c, const SDL_FRect& a) {
    const float pad = 24.0f;
    float x = a.x + pad, y = a.y + pad;

    c.text(x, y, "Keyboard", colors::text, 2.5f);
    y += 30;
    char mods[128] = "";
    auto add = [&](const char* n) { if (*mods) SDL_strlcat(mods, " ", sizeof mods); SDL_strlcat(mods, n, sizeof mods); };
    if (state_.mods & SDL_KMOD_SHIFT) add("Shift");
    if (state_.mods & SDL_KMOD_CTRL) add("Ctrl");
    if (state_.mods & SDL_KMOD_ALT) add("Alt");
    if (state_.mods & SDL_KMOD_GUI) add("Win/Cmd");
    if (state_.mods & SDL_KMOD_CAPS) add("CapsLock");
    if (state_.mods & SDL_KMOD_NUM) add("NumLock");
    c.textf(x, y, colors::dim, 1.4f, "Modifiers: %s", *mods ? mods : "none");
    y += 26;

    const float keyboardTop = y;
    const float pitch = 38.0f;
    const float gap = 3.0f;
    for (const KeyDef& k : ansiLayout()) {
        const float kx = x + k.x * pitch;
        const float ky = keyboardTop + k.y * pitch;
        const float kw = k.w * pitch - gap;
        const float kh = k.h * pitch - gap;
        const bool down = state_.pressed && k.scancode < state_.numKeys && state_.pressed[k.scancode];
        c.fillRect(kx, ky, kw, kh, down ? colors::accent : colors::panel);
        c.strokeRect(kx, ky, kw, kh, colors::outline);
        const std::string label = keyLabel(k, state_.mods);
        const float tw = Canvas::textWidth(label.c_str(), 1.1f);
        c.text(kx + std::max(2.0f, (kw - tw) / 2), ky + kh / 2 - 5, label.c_str(),
               down ? colors::bg : colors::text, 1.1f);
    }
    y = keyboardTop + ansiLayoutHeight() * pitch + 20;

    // ---- Last key panel.
    c.fillRect(x, y, 420, 60, colors::panel);
    c.strokeRect(x, y, 420, 60, colors::outline);
    if (state_.lastScancode != SDL_SCANCODE_UNKNOWN) {
        c.textf(x + 10, y + 8, colors::text, 1.4f, "Last key: %s%s",
                SDL_GetKeyName(state_.lastKey), state_.lastRepeat ? "  (repeat)" : "");
        c.textf(x + 10, y + 30, colors::dim, 1.2f, "scancode %s (%d)   keycode 0x%08x",
                SDL_GetScancodeName(state_.lastScancode), static_cast<int>(state_.lastScancode),
                static_cast<unsigned>(state_.lastKey));
    } else {
        c.text(x + 10, y + 20, "Press a key...", colors::dim, 1.3f);
    }
    y += 76;

    // ---- Text input box (exercises IME / dead-key composition).
    c.text(x, y, "Text input (type to test dead keys / IME):", colors::dim, 1.2f);
    y += 18;
    c.fillRect(x, y, 420, 34, colors::panel);
    c.strokeRect(x, y, 420, 34, colors::outline);
    c.text(x + 8, y + 9, state_.textInput.c_str(), colors::text, 1.3f);
}
