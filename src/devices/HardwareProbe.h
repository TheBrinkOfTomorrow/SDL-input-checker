#pragma once
#include <SDL3/SDL.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

// Looks at the OS's own view of attached hardware (IOKit on macOS, sysfs on
// Linux) and cross-checks it against what SDL enumerated, so the app can say
// *why* a controller that is plugged in does not show up in the device list.
//
// SDL only reports devices it can open; a controller that no driver claims,
// that is hidden by a hint, or that the user lacks permission for is simply
// absent. This probe fills that gap.

struct UsbInterface {
    Uint8 cls = 0, subclass = 0, protocol = 0;
    std::string driver;       // Linux: bound kernel driver ("usbhid", "xpad", ...). Empty if none / unknown.
    std::vector<std::string> eventNodes;  // Linux: /dev/input/eventN nodes created for this interface
};

struct UsbDevice {
    Uint16 vendor = 0, product = 0;
    Uint8 cls = 0, subclass = 0, protocol = 0;
    std::string name;
    std::string manufacturer;
    std::vector<UsbInterface> interfaces;
};

enum class FindingLevel { Info, Warning, Problem };

struct Finding {
    FindingLevel level = FindingLevel::Info;
    std::string device;   // short device label, e.g. "Logitech F710 (046d:c21f)"
    std::string summary;  // one line: what is wrong
    std::string detail;   // why, and what to do about it
};

class HardwareProbe {
public:
    // Re-scans USB hardware and rebuilds the findings. Cheap enough to call
    // every couple of seconds; a USB device SDL can't see produces no SDL
    // hot-plug event, so polling is the only way to notice it.
    void refresh();

    const std::vector<Finding>& findings() const { return findings_; }
    const std::vector<UsbDevice>& usbDevices() const { return usb_; }

    // Multi-line plain-text report (used by --diagnose and the log).
    std::string report() const;
    // Just the "USB devices seen by the OS" part of the report (hubs omitted).
    std::string usbListing() const;

    static std::vector<UsbDevice> listUsbDevices();

    // A newly attached HID controller reaches SDL a moment after the OS sees it;
    // "not opened by SDL" is only reported once it has been present this long.
    void setGracePeriodMs(Uint64 ms) { graceMs_ = ms; }

private:
    std::vector<UsbDevice> usb_;
    std::vector<Finding> findings_;
    std::map<std::pair<Uint16, Uint16>, Uint64> firstSeen_;  // vid/pid -> SDL_GetTicks() when first listed
    Uint64 graceMs_ = 3000;
};
