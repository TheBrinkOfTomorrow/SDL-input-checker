#pragma once
#include <SDL3/SDL.h>

#include "devices/KeyboardState.h"
#include "views/View.h"

// Physical ANSI keyboard layout with pressed keys lit, modifier state,
// last-key panel, and a text-input box to check IME / dead-key output.
class KeyboardView : public View {
public:
    explicit KeyboardView(SDL_Window* window);
    ~KeyboardView() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void draw(Canvas& c, const SDL_FRect& area) override;

private:
    SDL_Window* window_;
    KeyboardState state_;
};
