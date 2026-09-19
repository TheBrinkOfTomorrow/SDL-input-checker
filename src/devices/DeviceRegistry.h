#pragma once
#include <SDL3/SDL.h>

#include <optional>
#include <string>
#include <vector>

#include "devices/DeviceId.h"

// Enumerates input devices, tracks hot-plug, and keeps the selected
// gamepad/joystick open. Keyboards and mice need no open handle.
class DeviceRegistry {
public:
    DeviceRegistry() = default;
    ~DeviceRegistry();
    DeviceRegistry(const DeviceRegistry&) = delete;
    DeviceRegistry& operator=(const DeviceRegistry&) = delete;

    void refresh();
    // Returns true if the device list changed.
    bool handleEvent(const SDL_Event& e);

    const std::vector<DeviceInfo>& devices() const { return devices_; }
    int count(DeviceKind k) const;

    void select(std::optional<DeviceId> id);
    // Select the gamepad or raw-joystick entry with this instance id, whichever kind it was listed as.
    void selectJoystickId(SDL_JoystickID id);
    std::optional<DeviceId> selected() const { return selected_; }
    const DeviceInfo* selectedInfo() const;

    SDL_Gamepad* gamepad() const { return gamepad_; }
    SDL_Joystick* joystick() const { return joystick_; }

    // Last notable message (device removed, open failure, ...). Cleared on read.
    std::string takeNotice();

private:
    void closeSelected();
    bool openSelected();
    static std::string describeGamepad(SDL_JoystickID id);
    static std::string describeJoystick(SDL_JoystickID id);

    std::vector<DeviceInfo> devices_;
    std::optional<DeviceId> selected_;
    SDL_Gamepad* gamepad_ = nullptr;
    SDL_Joystick* joystick_ = nullptr;
    std::string notice_;
};
