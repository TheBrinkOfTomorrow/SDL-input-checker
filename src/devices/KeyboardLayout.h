#pragma once
#include <SDL3/SDL.h>

#include <string>
#include <vector>

// One physical key: position/size in "key units" (1 unit ~= one normal
// keycap), a static short label, and whether the label should instead be
// looked up live from the current OS keyboard layout (for alnum/punctuation
// keys, so a non-US layout shows its own glyphs on an otherwise fixed,
// physical ANSI layout).
struct KeyDef {
    SDL_Scancode scancode;
    float x, y, w, h;  // key units
    const char* label;
    bool dynamicLabel;
};

// A fixed 104-key ANSI/US physical layout (function row, main block, nav
// cluster, arrow cluster, numpad). Coordinates are in key units; multiply by
// a pixel pitch to place on screen.
const std::vector<KeyDef>& ansiLayout();
float ansiLayoutWidth();
float ansiLayoutHeight();

// Short label for a scancode, preferring the live OS layout for keys marked
// dynamicLabel (falls back to the static label if that lookup is empty).
std::string keyLabel(const KeyDef& k, SDL_Keymod mods);
