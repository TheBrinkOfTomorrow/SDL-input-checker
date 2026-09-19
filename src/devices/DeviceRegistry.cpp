#include "devices/DeviceRegistry.h"

#include <algorithm>
#include <cstdio>
#include <memory>

namespace {

template <typename T>
using SdlArray = std::unique_ptr<T, decltype(&SDL_free)>;

template <typename T>
SdlArray<T> sdlArray(T* p) { return SdlArray<T>(p, &SDL_free); }

const char* gamepadTypeName(SDL_GamepadType t) {
    switch (t) {
        case SDL_GAMEPAD_TYPE_XBOX360: return "Xbox 360";
        case SDL_GAMEPAD_TYPE_XBOXONE: return "Xbox One";
        case SDL_GAMEPAD_TYPE_PS3: return "PS3";
        case SDL_GAMEPAD_TYPE_PS4: return "PS4";
        case SDL_GAMEPAD_TYPE_PS5: return "PS5";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO: return "Switch Pro";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT: return "Joy-Con L";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT: return "Joy-Con R";
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: return "Joy-Con pair";
        case SDL_GAMEPAD_TYPE_STANDARD: return "Standard";
        default: return "Unknown";
    }
}

std::string vidPid(Uint16 vendor, Uint16 product) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04x:%04x", vendor, product);
    return buf;
}

}  // namespace

const char* deviceKindName(DeviceKind k) {
    switch (k) {
        case DeviceKind::Gamepad: return "Gamepad";
        case DeviceKind::Joystick: return "Joystick";
        case DeviceKind::Keyboard: return "Keyboard";
        case DeviceKind::Mouse: return "Mouse";
    }
    return "?";
}

DeviceRegistry::~DeviceRegistry() { closeSelected(); }

std::string DeviceRegistry::describeGamepad(SDL_JoystickID id) {
    std::string s = gamepadTypeName(SDL_GetGamepadTypeForID(id));
    s += "  " + vidPid(SDL_GetGamepadVendorForID(id), SDL_GetGamepadProductForID(id));
    const int player = SDL_GetGamepadPlayerIndexForID(id);
    if (player >= 0) s += "  P" + std::to_string(player + 1);
    return s;
}

std::string DeviceRegistry::describeJoystick(SDL_JoystickID id) {
    std::string s = "Raw joystick  " +
                    vidPid(SDL_GetJoystickVendorForID(id), SDL_GetJoystickProductForID(id));
    return s;
}

void DeviceRegistry::refresh() {
    devices_.clear();

    int n = 0;
    if (auto ids = sdlArray(SDL_GetGamepads(&n))) {
        for (int i = 0; i < n; ++i) {
            const char* name = SDL_GetGamepadNameForID(ids.get()[i]);
            devices_.push_back({{DeviceKind::Gamepad, ids.get()[i]},
                                name ? name : "Unnamed gamepad", describeGamepad(ids.get()[i])});
        }
    }
    if (auto ids = sdlArray(SDL_GetJoysticks(&n))) {
        for (int i = 0; i < n; ++i) {
            if (SDL_IsGamepad(ids.get()[i])) continue;  // already listed above
            const char* name = SDL_GetJoystickNameForID(ids.get()[i]);
            devices_.push_back({{DeviceKind::Joystick, ids.get()[i]},
                                name ? name : "Unnamed joystick", describeJoystick(ids.get()[i])});
        }
    }
    if (auto ids = sdlArray(SDL_GetKeyboards(&n))) {
        for (int i = 0; i < n; ++i) {
            const char* name = SDL_GetKeyboardNameForID(ids.get()[i]);
            devices_.push_back({{DeviceKind::Keyboard, ids.get()[i]},
                                name && *name ? name : "Keyboard", "Enumerated keyboard"});
        }
    }
    devices_.push_back({{DeviceKind::Keyboard, 0}, "System keyboard", "All keyboards (global state)", true});
    if (auto ids = sdlArray(SDL_GetMice(&n))) {
        for (int i = 0; i < n; ++i) {
            const char* name = SDL_GetMouseNameForID(ids.get()[i]);
            devices_.push_back({{DeviceKind::Mouse, ids.get()[i]},
                                name && *name ? name : "Mouse", "Enumerated mouse"});
        }
    }
    devices_.push_back({{DeviceKind::Mouse, 0}, "System mouse", "All pointing devices (global state)", true});

    SDL_Log("Devices (%zu):", devices_.size());
    for (const DeviceInfo& d : devices_)
        SDL_Log("  [%s %u] %s  (%s)", deviceKindName(d.id.kind), d.id.id, d.name.c_str(), d.detail.c_str());

    // Drop the selection if its device vanished.
    if (selected_ && !selectedInfo()) {
        notice_ = std::string(deviceKindName(selected_->kind)) + " disconnected";
        SDL_Log("%s", notice_.c_str());
        closeSelected();
        selected_.reset();
    }
}

bool DeviceRegistry::handleEvent(const SDL_Event& e) {
    switch (e.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
        case SDL_EVENT_JOYSTICK_ADDED:
        case SDL_EVENT_JOYSTICK_REMOVED:
        case SDL_EVENT_KEYBOARD_ADDED:
        case SDL_EVENT_KEYBOARD_REMOVED:
        case SDL_EVENT_MOUSE_ADDED:
        case SDL_EVENT_MOUSE_REMOVED:
            SDL_Log("Device event 0x%x (id %u)", e.type, e.jdevice.which);
            refresh();
            return true;
        default:
            return false;
    }
}

int DeviceRegistry::count(DeviceKind k) const {
    return static_cast<int>(std::count_if(devices_.begin(), devices_.end(),
                                          [k](const DeviceInfo& d) { return d.id.kind == k; }));
}

const DeviceInfo* DeviceRegistry::selectedInfo() const {
    if (!selected_) return nullptr;
    auto it = std::find_if(devices_.begin(), devices_.end(),
                           [&](const DeviceInfo& d) { return d.id == *selected_; });
    return it == devices_.end() ? nullptr : &*it;
}

void DeviceRegistry::select(std::optional<DeviceId> id) {
    if (id == selected_) return;
    closeSelected();
    selected_ = id;
    if (selected_ && !openSelected()) {
        notice_ = std::string("Failed to open ") + deviceKindName(selected_->kind) + ": " + SDL_GetError();
        SDL_Log("%s", notice_.c_str());
        selected_.reset();
    }
}

void DeviceRegistry::selectJoystickId(SDL_JoystickID id) {
    for (const DeviceInfo& d : devices_) {
        if ((d.id.kind == DeviceKind::Gamepad || d.id.kind == DeviceKind::Joystick) && d.id.id == id) {
            select(d.id);
            return;
        }
    }
}

bool DeviceRegistry::openSelected() {
    switch (selected_->kind) {
        case DeviceKind::Gamepad:
            gamepad_ = SDL_OpenGamepad(selected_->id);
            if (!gamepad_) return false;
            joystick_ = SDL_GetGamepadJoystick(gamepad_);
            for (auto sensor : {SDL_SENSOR_GYRO, SDL_SENSOR_ACCEL}) {
                if (SDL_GamepadHasSensor(gamepad_, sensor)) SDL_SetGamepadSensorEnabled(gamepad_, sensor, true);
            }
            SDL_Log("Opened gamepad '%s'", SDL_GetGamepadName(gamepad_));
            return true;
        case DeviceKind::Joystick:
            joystick_ = SDL_OpenJoystick(selected_->id);
            if (!joystick_) return false;
            SDL_Log("Opened joystick '%s'", SDL_GetJoystickName(joystick_));
            return true;
        case DeviceKind::Keyboard:
        case DeviceKind::Mouse:
            return true;
    }
    return false;
}

void DeviceRegistry::closeSelected() {
    if (gamepad_) {
        SDL_CloseGamepad(gamepad_);  // also releases the underlying joystick
        gamepad_ = nullptr;
        joystick_ = nullptr;
    } else if (joystick_) {
        SDL_CloseJoystick(joystick_);
        joystick_ = nullptr;
    }
}

std::string DeviceRegistry::takeNotice() {
    std::string s;
    s.swap(notice_);
    return s;
}
