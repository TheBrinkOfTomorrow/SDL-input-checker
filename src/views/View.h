#pragma once
#include <SDL3/SDL.h>

#include "draw/Canvas.h"

// A visualiser for one selected device. `area` is in window points.
class View {
public:
    virtual ~View() = default;
    virtual void handleEvent(const SDL_Event&) {}
    virtual void update(float dt) = 0;
    virtual void draw(Canvas& canvas, const SDL_FRect& area) = 0;
};
