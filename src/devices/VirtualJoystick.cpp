#include "devices/VirtualJoystick.h"

#include <cmath>

VirtualJoystick::~VirtualJoystick() {
    if (js_) SDL_CloseJoystick(js_);
    if (id_) SDL_DetachVirtualJoystick(id_);
}

bool VirtualJoystick::attach() {
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.vendor_id = 0x1234;
    desc.product_id = 0x5678;
    desc.naxes = 6;
    desc.nbuttons = 12;
    desc.nhats = 1;
    desc.name = "Virtual test joystick";
    id_ = SDL_AttachVirtualJoystick(&desc);
    if (!id_) {
        SDL_Log("SDL_AttachVirtualJoystick failed: %s", SDL_GetError());
        return false;
    }
    js_ = SDL_OpenJoystick(id_);  // needed to feed values
    return js_ != nullptr;
}

void VirtualJoystick::animate(float t) {
    if (!js_) return;
    for (int i = 0; i < 6; ++i) {
        const float v = std::sin(t * (0.7f + 0.3f * static_cast<float>(i)) + static_cast<float>(i));
        SDL_SetJoystickVirtualAxis(js_, i, static_cast<Sint16>(v * 32767.0f));
    }
    const int lit = static_cast<int>(t * 4.0f) % 12;
    for (int b = 0; b < 12; ++b) {
        if (!SDL_SetJoystickVirtualButton(js_, b, b == lit)) SDL_Log("virtual button %d: %s", b, SDL_GetError());
    }
    static const Uint8 dirs[] = {SDL_HAT_UP, SDL_HAT_RIGHTUP, SDL_HAT_RIGHT, SDL_HAT_RIGHTDOWN,
                                 SDL_HAT_DOWN, SDL_HAT_LEFTDOWN, SDL_HAT_LEFT, SDL_HAT_LEFTUP};
    if (!SDL_SetJoystickVirtualHat(js_, 0, dirs[static_cast<int>(t * 2.0f) % 8])) SDL_Log("virtual hat: %s", SDL_GetError());
}
