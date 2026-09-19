#include "draw/Canvas.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

void Canvas::fillRect(float x, float y, float w, float h, Color c) {
    color(c);
    const SDL_FRect rc{x * s_, y * s_, w * s_, h * s_};
    SDL_RenderFillRect(r_, &rc);
}

void Canvas::strokeRect(float x, float y, float w, float h, Color c) {
    color(c);
    const SDL_FRect rc{x * s_, y * s_, w * s_, h * s_};
    SDL_RenderRect(r_, &rc);
}

void Canvas::line(float x0, float y0, float x1, float y1, Color c) {
    color(c);
    SDL_RenderLine(r_, x0 * s_, y0 * s_, x1 * s_, y1 * s_);
}

void Canvas::fillCircle(float cx, float cy, float radius, Color c) {
    // Scanline fill: one horizontal line per pixel row.
    color(c);
    const float R = radius * s_;
    const float X = cx * s_, Y = cy * s_;
    const int rows = static_cast<int>(std::ceil(R));
    std::vector<SDL_FRect> rects;
    rects.reserve(static_cast<size_t>(rows) * 2 + 1);
    for (int dy = -rows; dy <= rows; ++dy) {
        const float fy = static_cast<float>(dy);
        const float half = std::sqrt(std::max(0.0f, R * R - fy * fy));
        rects.push_back({X - half, Y + fy, half * 2.0f, 1.0f});
    }
    SDL_RenderFillRects(r_, rects.data(), static_cast<int>(rects.size()));
}

void Canvas::strokeCircle(float cx, float cy, float radius, Color c, int segments) {
    color(c);
    std::vector<SDL_FPoint> pts;
    pts.reserve(static_cast<size_t>(segments) + 1);
    for (int i = 0; i <= segments; ++i) {
        const float a = static_cast<float>(i) / static_cast<float>(segments) * 6.2831853f;
        pts.push_back({(cx + std::cos(a) * radius) * s_, (cy + std::sin(a) * radius) * s_});
    }
    SDL_RenderLines(r_, pts.data(), static_cast<int>(pts.size()));
}

void Canvas::text(float x, float y, const char* s, Color c, float size) {
    const float k = size * s_;
    SDL_SetRenderScale(r_, k, k);
    color(c);
    SDL_RenderDebugText(r_, x * s_ / k, y * s_ / k, s);
    SDL_SetRenderScale(r_, 1.0f, 1.0f);
}

void Canvas::textf(float x, float y, Color c, float size, const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    text(x, y, buf, c, size);
}

float Canvas::textWidth(const char* s, float size) {
    return static_cast<float>(std::strlen(s)) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * size;
}
