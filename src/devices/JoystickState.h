#pragma once
#include <SDL3/SDL.h>

#include <vector>

// Raw per-frame snapshot of an SDL_Joystick.
struct JoystickState {
    std::vector<Sint16> axes;
    std::vector<bool> buttons;
    std::vector<Uint8> hats;
    std::vector<SDL_Point> balls;  // accumulated deltas since last sample
    SDL_PowerState power = SDL_POWERSTATE_UNKNOWN;
    int batteryPercent = -1;

    void sample(SDL_Joystick* js) {
        axes.assign(static_cast<size_t>(SDL_GetNumJoystickAxes(js)), 0);
        for (size_t i = 0; i < axes.size(); ++i) axes[i] = SDL_GetJoystickAxis(js, static_cast<int>(i));
        buttons.assign(static_cast<size_t>(SDL_GetNumJoystickButtons(js)), false);
        for (size_t i = 0; i < buttons.size(); ++i) buttons[i] = SDL_GetJoystickButton(js, static_cast<int>(i));
        hats.assign(static_cast<size_t>(SDL_GetNumJoystickHats(js)), SDL_HAT_CENTERED);
        for (size_t i = 0; i < hats.size(); ++i) hats[i] = SDL_GetJoystickHat(js, static_cast<int>(i));
        balls.assign(static_cast<size_t>(SDL_GetNumJoystickBalls(js)), SDL_Point{0, 0});
        for (size_t i = 0; i < balls.size(); ++i) SDL_GetJoystickBall(js, static_cast<int>(i), &balls[i].x, &balls[i].y);
        power = SDL_GetJoystickPowerInfo(js, &batteryPercent);
    }
};
