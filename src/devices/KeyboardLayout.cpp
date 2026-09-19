#include "devices/KeyboardLayout.h"

namespace {

using SC = SDL_Scancode;

// Row-cursor builder for the simple (non-spanning) rows: main block and the
// nav cluster's top three rows.
struct RowBuilder {
    std::vector<KeyDef>& out;
    float x, y;
    RowBuilder(std::vector<KeyDef>& o, float startX, float startY) : out(o), x(startX), y(startY) {}
    void key(SC sc, float w, const char* label, bool dynamic = false) {
        out.push_back({sc, x, y, w, 1.0f, label, dynamic});
        x += w;
    }
    void gap(float w) { x += w; }
};

std::vector<KeyDef> build() {
    std::vector<KeyDef> k;
    k.reserve(110);

    // ---- Function row (y = 0).
    {
        RowBuilder r(k, 0, 0);
        r.key(SDL_SCANCODE_ESCAPE, 1, "Esc");
        r.gap(1);
        r.key(SDL_SCANCODE_F1, 1, "F1"); r.key(SDL_SCANCODE_F2, 1, "F2");
        r.key(SDL_SCANCODE_F3, 1, "F3"); r.key(SDL_SCANCODE_F4, 1, "F4");
        r.gap(0.5f);
        r.key(SDL_SCANCODE_F5, 1, "F5"); r.key(SDL_SCANCODE_F6, 1, "F6");
        r.key(SDL_SCANCODE_F7, 1, "F7"); r.key(SDL_SCANCODE_F8, 1, "F8");
        r.gap(0.5f);
        r.key(SDL_SCANCODE_F9, 1, "F9"); r.key(SDL_SCANCODE_F10, 1, "F10");
        r.key(SDL_SCANCODE_F11, 1, "F11"); r.key(SDL_SCANCODE_F12, 1, "F12");
    }
    // ---- Number row (y = 1.5).
    {
        RowBuilder r(k, 0, 1.5f);
        r.key(SDL_SCANCODE_GRAVE, 1, "`", true);
        r.key(SDL_SCANCODE_1, 1, "1", true); r.key(SDL_SCANCODE_2, 1, "2", true);
        r.key(SDL_SCANCODE_3, 1, "3", true); r.key(SDL_SCANCODE_4, 1, "4", true);
        r.key(SDL_SCANCODE_5, 1, "5", true); r.key(SDL_SCANCODE_6, 1, "6", true);
        r.key(SDL_SCANCODE_7, 1, "7", true); r.key(SDL_SCANCODE_8, 1, "8", true);
        r.key(SDL_SCANCODE_9, 1, "9", true); r.key(SDL_SCANCODE_0, 1, "0", true);
        r.key(SDL_SCANCODE_MINUS, 1, "-", true); r.key(SDL_SCANCODE_EQUALS, 1, "=", true);
        r.key(SDL_SCANCODE_BACKSPACE, 2, "Back");
    }
    // ---- Tab row (y = 2.5).
    {
        RowBuilder r(k, 0, 2.5f);
        r.key(SDL_SCANCODE_TAB, 1.5f, "Tab");
        for (SC sc : {SC(SDL_SCANCODE_Q), SC(SDL_SCANCODE_W), SC(SDL_SCANCODE_E), SC(SDL_SCANCODE_R),
                      SC(SDL_SCANCODE_T), SC(SDL_SCANCODE_Y), SC(SDL_SCANCODE_U), SC(SDL_SCANCODE_I),
                      SC(SDL_SCANCODE_O), SC(SDL_SCANCODE_P)})
            r.key(sc, 1, "", true);
        r.key(SDL_SCANCODE_LEFTBRACKET, 1, "[", true);
        r.key(SDL_SCANCODE_RIGHTBRACKET, 1, "]", true);
        r.key(SDL_SCANCODE_BACKSLASH, 1.5f, "\\", true);
    }
    // ---- Caps row (y = 3.5).
    {
        RowBuilder r(k, 0, 3.5f);
        r.key(SDL_SCANCODE_CAPSLOCK, 1.75f, "Caps");
        for (SC sc : {SC(SDL_SCANCODE_A), SC(SDL_SCANCODE_S), SC(SDL_SCANCODE_D), SC(SDL_SCANCODE_F),
                      SC(SDL_SCANCODE_G), SC(SDL_SCANCODE_H), SC(SDL_SCANCODE_J), SC(SDL_SCANCODE_K),
                      SC(SDL_SCANCODE_L)})
            r.key(sc, 1, "", true);
        r.key(SDL_SCANCODE_SEMICOLON, 1, ";", true);
        r.key(SDL_SCANCODE_APOSTROPHE, 1, "'", true);
        r.key(SDL_SCANCODE_RETURN, 2.25f, "Enter");
    }
    // ---- Shift row (y = 4.5).
    {
        RowBuilder r(k, 0, 4.5f);
        r.key(SDL_SCANCODE_LSHIFT, 2.25f, "Shift");
        for (SC sc : {SC(SDL_SCANCODE_Z), SC(SDL_SCANCODE_X), SC(SDL_SCANCODE_C), SC(SDL_SCANCODE_V),
                      SC(SDL_SCANCODE_B), SC(SDL_SCANCODE_N), SC(SDL_SCANCODE_M)})
            r.key(sc, 1, "", true);
        r.key(SDL_SCANCODE_COMMA, 1, ",", true);
        r.key(SDL_SCANCODE_PERIOD, 1, ".", true);
        r.key(SDL_SCANCODE_SLASH, 1, "/", true);
        r.key(SDL_SCANCODE_RSHIFT, 2.75f, "Shift");
    }
    // ---- Bottom row (y = 5.5).
    {
        RowBuilder r(k, 0, 5.5f);
        r.key(SDL_SCANCODE_LCTRL, 1.25f, "Ctrl");
        r.key(SDL_SCANCODE_LGUI, 1.25f, "Win");
        r.key(SDL_SCANCODE_LALT, 1.25f, "Alt");
        r.key(SDL_SCANCODE_SPACE, 6.25f, "Space");
        r.key(SDL_SCANCODE_RALT, 1.25f, "Alt");
        r.key(SDL_SCANCODE_RGUI, 1.25f, "Win");
        r.key(SDL_SCANCODE_APPLICATION, 1.25f, "Menu");
        r.key(SDL_SCANCODE_RCTRL, 1.25f, "Ctrl");
    }

    // ---- Nav cluster (x = 15.5).
    constexpr float navX = 15.5f;
    {
        RowBuilder r1(k, navX, 1.5f); r1.key(SDL_SCANCODE_PRINTSCREEN, 1, "PrSc");
        r1.key(SDL_SCANCODE_SCROLLLOCK, 1, "ScrLk"); r1.key(SDL_SCANCODE_PAUSE, 1, "Pause");
        RowBuilder r2(k, navX, 2.5f); r2.key(SDL_SCANCODE_INSERT, 1, "Ins");
        r2.key(SDL_SCANCODE_HOME, 1, "Home"); r2.key(SDL_SCANCODE_PAGEUP, 1, "PgUp");
        RowBuilder r3(k, navX, 3.5f); r3.key(SDL_SCANCODE_DELETE, 1, "Del");
        r3.key(SDL_SCANCODE_END, 1, "End"); r3.key(SDL_SCANCODE_PAGEDOWN, 1, "PgDn");
    }
    // ---- Arrow cluster (rows 4-5 under the nav cluster).
    k.push_back({SDL_SCANCODE_UP, navX + 1, 4.5f, 1, 1, "^", false});
    k.push_back({SDL_SCANCODE_LEFT, navX, 5.5f, 1, 1, "<", false});
    k.push_back({SDL_SCANCODE_DOWN, navX + 1, 5.5f, 1, 1, "v", false});
    k.push_back({SDL_SCANCODE_RIGHT, navX + 2, 5.5f, 1, 1, ">", false});

    // ---- Numpad (x = 19).
    constexpr float npX = 19.0f;
    k.push_back({SDL_SCANCODE_NUMLOCKCLEAR, npX, 1.5f, 1, 1, "Num", false});
    k.push_back({SDL_SCANCODE_KP_DIVIDE, npX + 1, 1.5f, 1, 1, "/", false});
    k.push_back({SDL_SCANCODE_KP_MULTIPLY, npX + 2, 1.5f, 1, 1, "*", false});
    k.push_back({SDL_SCANCODE_KP_MINUS, npX + 3, 1.5f, 1, 1, "-", false});
    k.push_back({SDL_SCANCODE_KP_7, npX, 2.5f, 1, 1, "7", false});
    k.push_back({SDL_SCANCODE_KP_8, npX + 1, 2.5f, 1, 1, "8", false});
    k.push_back({SDL_SCANCODE_KP_9, npX + 2, 2.5f, 1, 1, "9", false});
    k.push_back({SDL_SCANCODE_KP_PLUS, npX + 3, 2.5f, 1, 2, "+", false});
    k.push_back({SDL_SCANCODE_KP_4, npX, 3.5f, 1, 1, "4", false});
    k.push_back({SDL_SCANCODE_KP_5, npX + 1, 3.5f, 1, 1, "5", false});
    k.push_back({SDL_SCANCODE_KP_6, npX + 2, 3.5f, 1, 1, "6", false});
    k.push_back({SDL_SCANCODE_KP_1, npX, 4.5f, 1, 1, "1", false});
    k.push_back({SDL_SCANCODE_KP_2, npX + 1, 4.5f, 1, 1, "2", false});
    k.push_back({SDL_SCANCODE_KP_3, npX + 2, 4.5f, 1, 1, "3", false});
    k.push_back({SDL_SCANCODE_KP_ENTER, npX + 3, 4.5f, 1, 2, "Ent", false});
    k.push_back({SDL_SCANCODE_KP_0, npX, 5.5f, 2, 1, "0", false});
    k.push_back({SDL_SCANCODE_KP_PERIOD, npX + 2, 5.5f, 1, 1, ".", false});

    return k;
}

}  // namespace

const std::vector<KeyDef>& ansiLayout() {
    static const std::vector<KeyDef> layout = build();
    return layout;
}

float ansiLayoutWidth() { return 23.0f; }
float ansiLayoutHeight() { return 6.5f; }

std::string keyLabel(const KeyDef& k, SDL_Keymod mods) {
    if (k.dynamicLabel) {
        const SDL_Keycode kc = SDL_GetKeyFromScancode(k.scancode, mods, false);
        if (const char* name = SDL_GetKeyName(kc); name && *name && !SDL_strchr(name, ' '))
            return name;
        if (const char* sname = SDL_GetScancodeName(k.scancode); sname && *sname) return sname;
    }
    return *k.label ? k.label : "?";
}
