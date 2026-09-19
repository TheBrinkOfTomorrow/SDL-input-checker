#pragma once
#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "devices/GamepadState.h"
#include "views/View.h"

// Gamepad silhouette: face buttons, D-pad, shoulders/triggers, sticks,
// start/back/guide, touchpad, sensors, battery, plus rumble/LED test buttons.
class GamepadView : public View {
public:
    explicit GamepadView(SDL_Gamepad* gp);
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void draw(Canvas& c, const SDL_FRect& area) override;

private:
    struct HitRect { SDL_FRect r; int action; };  // action: 0 rumble, 1 rumble triggers, 2 cycle LED

    void drawStick(Canvas& c, float cx, float cy, float radius, float x, float y, bool pressed, float glow);
    void drawTrigger(Canvas& c, float x, float y, float w, float h, float value, const char* label);
    void drawFaceButton(Canvas& c, float cx, float cy, float radius, SDL_GamepadButton btn, float glow);
    void drawSmallButton(Canvas& c, float cx, float cy, float radius, const char* label, bool pressed);
    void fireAction(int action);

    SDL_Gamepad* gp_;
    GamepadState state_;
    std::string title_, subtitle_, mapping_;
    std::vector<float> buttonGlow_;  // decaying highlight per SDL_GamepadButton
    std::vector<HitRect> hitRects_;  // recomputed each draw(), read back in handleEvent()
    float rumbleFlashTtl_ = 0.0f;
    int ledIndex_ = 0;
};
