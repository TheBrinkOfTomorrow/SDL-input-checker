#pragma once
#include <SDL3/SDL.h>

// Thin drawing helper. Coordinates are in window points; `scale` converts to
// output pixels so drawing code is identical on Retina and non-Retina displays.
struct Color { Uint8 r, g, b, a = 255; };

namespace colors {
inline constexpr Color bg{32, 35, 41};
inline constexpr Color panel{44, 48, 56};
inline constexpr Color idle{78, 84, 96};
inline constexpr Color outline{110, 118, 132};
inline constexpr Color text{215, 220, 228};
inline constexpr Color dim{130, 138, 150};
inline constexpr Color accent{255, 170, 60};
inline constexpr Color accent2{80, 200, 255};
inline constexpr Color good{110, 220, 120};

inline Color lerp(Color a, Color b, float t) {
    return {static_cast<Uint8>(a.r + (b.r - a.r) * t), static_cast<Uint8>(a.g + (b.g - a.g) * t),
            static_cast<Uint8>(a.b + (b.b - a.b) * t)};
}
}  // namespace colors

class Canvas {
public:
    Canvas(SDL_Renderer* r, float scale) : r_(r), s_(scale) {}

    void color(Color c) { SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a); }
    void fillRect(float x, float y, float w, float h, Color c);
    void strokeRect(float x, float y, float w, float h, Color c);
    void line(float x0, float y0, float x1, float y1, Color c);
    void fillCircle(float cx, float cy, float radius, Color c);
    void strokeCircle(float cx, float cy, float radius, Color c, int segments = 48);
    // size is a multiplier of the 8 px debug font.
    void text(float x, float y, const char* s, Color c, float size = 1.5f);
    void textf(float x, float y, Color c, float size, const char* fmt, ...);
    // Width in points of a string at the given size.
    static float textWidth(const char* s, float size);

    SDL_Renderer* renderer() const { return r_; }
    float scale() const { return s_; }

private:
    SDL_Renderer* r_;
    float s_;
};
