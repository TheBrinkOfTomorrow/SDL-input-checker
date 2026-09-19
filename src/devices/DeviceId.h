#pragma once
#include <SDL3/SDL.h>

#include <string>

enum class DeviceKind { Gamepad, Joystick, Keyboard, Mouse };

const char* deviceKindName(DeviceKind k);

// Identity of a device: kind + SDL instance id. id == 0 marks the synthetic
// "system" keyboard / mouse entries that are always present.
struct DeviceId {
    DeviceKind kind = DeviceKind::Gamepad;
    Uint32 id = 0;

    bool operator==(const DeviceId&) const = default;
};

struct DeviceInfo {
    DeviceId id;
    std::string name;    // display name
    std::string detail;  // secondary line: type, vendor/product, connection, ...
    bool synthetic = false;
};
