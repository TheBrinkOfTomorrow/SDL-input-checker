#pragma once
#include <SDL3/SDL.h>

#include <algorithm>
#include <deque>

// Per-frame snapshot of the mouse: button mask, absolute window position,
// relative motion accumulated since the last sample, wheel deltas (decaying
// for display), and a short trail of recent positions.
struct MouseState {
    SDL_MouseButtonFlags buttons = 0;
    float x = 0, y = 0;
    float relX = 0, relY = 0;   // accumulated since last sample()
    float wheelX = 0, wheelY = 0;  // decaying display values
    std::deque<SDL_FPoint> trail;

    void onMotion(const SDL_MouseMotionEvent& e) {
        relX += e.xrel;
        relY += e.yrel;
    }
    void onWheel(const SDL_MouseWheelEvent& e) {
        wheelX += e.x;
        wheelY += e.y;
    }

    void sample(SDL_Window* window, float dt) {
        buttons = SDL_GetMouseState(&x, &y);
        (void)window;
        trail.push_back({x, y});
        while (trail.size() > 60) trail.pop_front();
        const float decay = std::max(0.0f, 1.0f - dt * 3.0f);
        wheelX *= decay;
        wheelY *= decay;
    }
    void endFrame() { relX = relY = 0; }
};
