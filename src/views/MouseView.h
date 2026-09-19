#pragma once
#include <SDL3/SDL.h>

#include "devices/MouseState.h"
#include "views/View.h"

// Mouse silhouette with buttons lit, a wheel meter, an XY motion pad with a
// trail, numeric read-out, and a toggle for relative (captured) mouse mode.
class MouseView : public View {
public:
    explicit MouseView(SDL_Window* window);
    ~MouseView() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void draw(Canvas& c, const SDL_FRect& area) override;

private:
    SDL_Window* window_;
    MouseState state_;
    SDL_FRect toggleRect_{};
    bool relativeMode_ = false;
};
