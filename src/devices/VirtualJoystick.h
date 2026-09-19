#pragma once
#include <SDL3/SDL.h>

// Developer aid: an SDL virtual joystick with animated axes/buttons/hat so the
// views can be exercised without hardware (run with --virtual).
class VirtualJoystick {
public:
    ~VirtualJoystick();
    bool attach();
    void animate(float t);
    SDL_JoystickID id() const { return id_; }

private:
    SDL_JoystickID id_ = 0;
    SDL_Joystick* js_ = nullptr;
};
