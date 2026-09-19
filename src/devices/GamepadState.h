#pragma once
#include <SDL3/SDL.h>

#include <array>
#include <vector>

// Per-frame snapshot of an SDL_Gamepad: buttons, axes, touchpad fingers,
// sensors (if enabled), battery, and static capability flags.
struct GamepadTouch {
    bool down = false;
    float x = 0, y = 0, pressure = 0;
};

struct GamepadState {
    std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttons{};
    std::array<float, SDL_GAMEPAD_AXIS_COUNT> axes{};  // -1..1, triggers 0..1

    std::vector<std::vector<GamepadTouch>> touchpads;  // [touchpad][finger]

    bool hasGyro = false, hasAccel = false;
    float gyro[3] = {0, 0, 0};   // rad/s
    float accel[3] = {0, 0, 0};  // m/s^2

    SDL_PowerState power = SDL_POWERSTATE_UNKNOWN;
    int batteryPercent = -1;

    bool hasMonoLed = false, hasRgbLed = false, hasPlayerLed = false;
    bool hasRumble = false, hasRumbleTriggers = false;

    void sampleCapabilities(SDL_Gamepad* gp) {
        touchpads.resize(static_cast<size_t>(SDL_GetNumGamepadTouchpads(gp)));
        hasGyro = SDL_GamepadHasSensor(gp, SDL_SENSOR_GYRO);
        hasAccel = SDL_GamepadHasSensor(gp, SDL_SENSOR_ACCEL);
        const SDL_PropertiesID props = SDL_GetGamepadProperties(gp);
        hasMonoLed = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_MONO_LED_BOOLEAN, false);
        hasRgbLed = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN, false);
        hasPlayerLed = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_PLAYER_LED_BOOLEAN, false);
        hasRumble = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
        hasRumbleTriggers = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_TRIGGER_RUMBLE_BOOLEAN, false);
    }

    void sample(SDL_Gamepad* gp) {
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b)
            buttons[static_cast<size_t>(b)] = SDL_GetGamepadButton(gp, static_cast<SDL_GamepadButton>(b));
        for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) {
            const Sint16 raw = SDL_GetGamepadAxis(gp, static_cast<SDL_GamepadAxis>(a));
            const bool isTrigger = a == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || a == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
            axes[static_cast<size_t>(a)] = isTrigger ? static_cast<float>(raw) / 32767.0f
                                                      : static_cast<float>(raw) / (raw < 0 ? 32768.0f : 32767.0f);
        }
        for (size_t t = 0; t < touchpads.size(); ++t) {
            const int nf = SDL_GetNumGamepadTouchpadFingers(gp, static_cast<int>(t));
            touchpads[t].assign(static_cast<size_t>(nf), {});
            for (int f = 0; f < nf; ++f) {
                GamepadTouch& ft = touchpads[t][static_cast<size_t>(f)];
                SDL_GetGamepadTouchpadFinger(gp, static_cast<int>(t), f, &ft.down, &ft.x, &ft.y, &ft.pressure);
            }
        }
        if (hasGyro) SDL_GetGamepadSensorData(gp, SDL_SENSOR_GYRO, gyro, 3);
        if (hasAccel) SDL_GetGamepadSensorData(gp, SDL_SENSOR_ACCEL, accel, 3);
        power = SDL_GetGamepadPowerInfo(gp, &batteryPercent);
    }
};
