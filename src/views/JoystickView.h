#pragma once
#include <string>

#include "devices/JoystickState.h"
#include "views/View.h"

// Generic raw view: axis bars, button grid, hat indicators, ball deltas.
// Works for any SDL_Joystick, including the one behind an opened gamepad.
class JoystickView : public View {
public:
    explicit JoystickView(SDL_Joystick* js);
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void draw(Canvas& c, const SDL_FRect& area) override;

private:
    SDL_Joystick* js_;
    JoystickState state_;
    std::string title_, guid_, info_;
    std::vector<float> axisPeak_;   // decaying max |value| per axis
    std::vector<float> buttonGlow_; // decays after release so taps stay visible
};
