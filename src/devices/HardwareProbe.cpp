#include "devices/HardwareProbe.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <utility>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#elif defined(__linux__)
#include <unistd.h>

#include <filesystem>
#include <fstream>
#endif

namespace {

std::string hex4(Uint16 v) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "%04x", v);
    return buf;
}

std::string label(const UsbDevice& d, const char* knownName = nullptr) {
    std::string s = knownName ? knownName : (d.name.empty() ? std::string("USB device") : d.name);
    return s + " (" + hex4(d.vendor) + ":" + hex4(d.product) + ")";
}

// Controllers whose USB identity depends on a mode switch. In "X" mode they
// speak Microsoft's XInput (Xbox 360) protocol over a vendor-specific USB
// class; in "D" mode they are an ordinary HID gamepad.
struct ModeSwitchPad {
    Uint16 vendor, xinputPid, dinputPid;
    const char* name;
};
constexpr ModeSwitchPad kModeSwitchPads[] = {
    {0x046d, 0xc21d, 0xc216, "Logitech F310"},
    {0x046d, 0xc21e, 0xc218, "Logitech F510"},
    {0x046d, 0xc21f, 0xc219, "Logitech F710"},
};

const ModeSwitchPad* modeSwitchPad(Uint16 vid, Uint16 pid) {
    for (const auto& p : kModeSwitchPads)
        if (p.vendor == vid && (p.xinputPid == pid || p.dinputPid == pid)) return &p;
    return nullptr;
}

// XInput-style USB: Xbox 360 wired/wireless-receiver pads use device or interface
// class 0xFF/0x5D; Xbox One pads use interface 0xFF/0x47/0xD0. Many third-party
// pads in "X" mode also report device class 0xFF/0xFF/0xFF with no interfaces
// visible until a driver configures them.
bool isXInputClass(const UsbDevice& d) {
    if (d.cls == 0xFF && (d.subclass == 0x5D || d.subclass == 0x47)) return true;
    for (const auto& i : d.interfaces)
        if (i.cls == 0xFF && (i.subclass == 0x5D || (i.subclass == 0x47 && i.protocol == 0xD0))) return true;
    // Unconfigured vendor-class device (no interfaces published yet) whose name says it's a pad.
    if (d.cls == 0xFF && d.subclass == 0xFF && d.protocol == 0xFF) {
        std::string n = d.name;
        std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(SDL_tolower(c)); });
        for (const char* w : {"gamepad", "controller", "joystick", "xbox"})
            if (n.find(w) != std::string::npos) return true;
    }
    return false;
}

struct HidGamepadIds {
    std::set<std::pair<Uint16, Uint16>> ids;  // vid/pid of HID collections with joystick/gamepad usage
};

HidGamepadIds hidGamepads() {
    HidGamepadIds out;
    if (SDL_hid_device_info* list = SDL_hid_enumerate(0, 0)) {
        for (SDL_hid_device_info* h = list; h; h = h->next) {
            // Generic Desktop page, Joystick (4) / Game Pad (5) / Multi-axis (8).
            if (h->usage_page == 0x01 && (h->usage == 0x04 || h->usage == 0x05 || h->usage == 0x08))
                out.ids.insert({h->vendor_id, h->product_id});
        }
        SDL_hid_free_enumeration(list);
    }
    return out;
}

bool hintIgnores(const char* hint, Uint16 vid, Uint16 pid) {
    const char* v = SDL_GetHint(hint);
    if (!v || !*v) return false;
    char needle[16];
    std::snprintf(needle, sizeof needle, "0x%04x/0x%04x", vid, pid);
    std::string hay(v);
    std::transform(hay.begin(), hay.end(), hay.begin(), [](unsigned char c) { return static_cast<char>(SDL_tolower(c)); });
    return hay.find(needle) != std::string::npos;
}

#if defined(__APPLE__)

int intProp(io_registry_entry_t e, const char* key) {
    int value = -1;
    CFStringRef k = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
    if (CFTypeRef ref = IORegistryEntryCreateCFProperty(e, k, kCFAllocatorDefault, 0)) {
        if (CFGetTypeID(ref) == CFNumberGetTypeID()) CFNumberGetValue(static_cast<CFNumberRef>(ref), kCFNumberIntType, &value);
        CFRelease(ref);
    }
    CFRelease(k);
    return value;
}

std::string strProp(io_registry_entry_t e, const char* key) {
    std::string out;
    CFStringRef k = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
    if (CFTypeRef ref = IORegistryEntryCreateCFProperty(e, k, kCFAllocatorDefault, 0)) {
        if (CFGetTypeID(ref) == CFStringGetTypeID()) {
            char buf[256];
            if (CFStringGetCString(static_cast<CFStringRef>(ref), buf, sizeof buf, kCFStringEncodingUTF8)) out = buf;
        }
        CFRelease(ref);
    }
    CFRelease(k);
    // Some devices pad their strings with spaces.
    const auto b = out.find_first_not_of(' ');
    const auto e2 = out.find_last_not_of(' ');
    return b == std::string::npos ? std::string() : out.substr(b, e2 - b + 1);
}

#endif

}  // namespace

std::vector<UsbDevice> HardwareProbe::listUsbDevices() {
    std::vector<UsbDevice> out;
#if defined(__APPLE__)
    io_iterator_t it = IO_OBJECT_NULL;
    // MACH_PORT_NULL selects the default main port on every macOS version.
    if (IOServiceGetMatchingServices(MACH_PORT_NULL, IOServiceMatching("IOUSBHostDevice"), &it) != KERN_SUCCESS)
        return out;
    while (io_service_t dev = IOIteratorNext(it)) {
        UsbDevice d;
        d.vendor = static_cast<Uint16>(intProp(dev, "idVendor"));
        d.product = static_cast<Uint16>(intProp(dev, "idProduct"));
        d.cls = static_cast<Uint8>(intProp(dev, "bDeviceClass"));
        d.subclass = static_cast<Uint8>(intProp(dev, "bDeviceSubClass"));
        d.protocol = static_cast<Uint8>(intProp(dev, "bDeviceProtocol"));
        d.name = strProp(dev, "USB Product Name");
        if (d.name.empty()) d.name = strProp(dev, "kUSBProductString");
        d.manufacturer = strProp(dev, "USB Vendor Name");
        if (d.manufacturer.empty()) d.manufacturer = strProp(dev, "kUSBVendorString");

        io_iterator_t kids = IO_OBJECT_NULL;
        if (IORegistryEntryGetChildIterator(dev, kIOServicePlane, &kids) == KERN_SUCCESS) {
            while (io_registry_entry_t k = IOIteratorNext(kids)) {
                if (IOObjectConformsTo(k, "IOUSBHostInterface")) {
                    UsbInterface i;
                    i.cls = static_cast<Uint8>(intProp(k, "bInterfaceClass"));
                    i.subclass = static_cast<Uint8>(intProp(k, "bInterfaceSubClass"));
                    i.protocol = static_cast<Uint8>(intProp(k, "bInterfaceProtocol"));
                    d.interfaces.push_back(i);
                }
                IOObjectRelease(k);
            }
            IOObjectRelease(kids);
        }
        IOObjectRelease(dev);
        out.push_back(std::move(d));
    }
    IOObjectRelease(it);
#elif defined(__linux__)
    namespace fs = std::filesystem;
    auto readStr = [](const fs::path& p) {
        std::ifstream f(p);
        std::string s;
        std::getline(f, s);
        return s;
    };
    auto readHex = [&](const fs::path& p) { return static_cast<unsigned>(std::strtoul(readStr(p).c_str(), nullptr, 16)); };
    std::error_code ec;
    const fs::path root = "/sys/bus/usb/devices";
    for (const auto& entry : fs::directory_iterator(root, ec)) {
        const fs::path dir = entry.path();
        if (!fs::exists(dir / "idVendor")) continue;  // interface dirs ("1-2:1.0") have no idVendor
        UsbDevice d;
        d.vendor = static_cast<Uint16>(readHex(dir / "idVendor"));
        d.product = static_cast<Uint16>(readHex(dir / "idProduct"));
        d.cls = static_cast<Uint8>(readHex(dir / "bDeviceClass"));
        d.subclass = static_cast<Uint8>(readHex(dir / "bDeviceSubClass"));
        d.protocol = static_cast<Uint8>(readHex(dir / "bDeviceProtocol"));
        d.name = readStr(dir / "product");
        d.manufacturer = readStr(dir / "manufacturer");
        const std::string base = dir.filename().string() + ":";
        for (const auto& sub : fs::directory_iterator(dir, ec)) {
            const std::string n = sub.path().filename().string();
            if (n.rfind(base, 0) != 0 || !fs::exists(sub.path() / "bInterfaceClass")) continue;
            UsbInterface i;
            i.cls = static_cast<Uint8>(readHex(sub.path() / "bInterfaceClass"));
            i.subclass = static_cast<Uint8>(readHex(sub.path() / "bInterfaceSubClass"));
            i.protocol = static_cast<Uint8>(readHex(sub.path() / "bInterfaceProtocol"));
            if (fs::is_symlink(sub.path() / "driver", ec))
                i.driver = fs::read_symlink(sub.path() / "driver", ec).filename().string();
            // eventN nodes live under <iface>/[...]/input/inputM/eventN; HID adds a hidraw/<bus:vid:pid.n> level.
            for (auto rit = fs::recursive_directory_iterator(sub.path(), fs::directory_options::none, ec);
                 rit != fs::recursive_directory_iterator(); rit.increment(ec)) {
                if (ec) break;
                if (rit.depth() > 4) { rit.disable_recursion_pending(); continue; }
                if (rit->is_symlink(ec)) { rit.disable_recursion_pending(); continue; }
                const std::string fn = rit->path().filename().string();
                if (fn.rfind("event", 0) == 0 && rit->is_directory(ec)) i.eventNodes.push_back("/dev/input/" + fn);
            }
            d.interfaces.push_back(std::move(i));
        }
        out.push_back(std::move(d));
    }
#endif
    return out;
}

void HardwareProbe::refresh() {
    usb_ = listUsbDevices();
    findings_.clear();

    // What SDL already enumerated, by vendor/product.
    std::set<std::pair<Uint16, Uint16>> sdlJoysticks, sdlGamepads;
    int n = 0;
    if (SDL_JoystickID* ids = SDL_GetJoysticks(&n)) {
        for (int i = 0; i < n; ++i) {
            const std::pair<Uint16, Uint16> vp{SDL_GetJoystickVendorForID(ids[i]), SDL_GetJoystickProductForID(ids[i])};
            sdlJoysticks.insert(vp);
            if (SDL_IsGamepad(ids[i])) sdlGamepads.insert(vp);
        }
        SDL_free(ids);
    }
    const HidGamepadIds hid = hidGamepads();

    for (const UsbDevice& d : usb_) {
        const std::pair<Uint16, Uint16> vp{d.vendor, d.product};
        const ModeSwitchPad* msp = modeSwitchPad(d.vendor, d.product);
        const bool xinput = isXInputClass(d) || (msp && msp->xinputPid == d.product);
        const bool hidPad = hid.ids.count(vp) > 0;
        const bool seen = sdlJoysticks.count(vp) > 0;
        const char* known = msp ? msp->name : nullptr;

        if (!xinput && !hidPad && !msp) continue;  // not a game controller as far as we can tell

        if (hintIgnores(SDL_HINT_JOYSTICK_BLACKLIST_DEVICES, d.vendor, d.product) ||
            hintIgnores(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES, d.vendor, d.product)) {
            findings_.push_back({FindingLevel::Problem, label(d, known), "Hidden by an SDL hint",
                                 "This device's vendor/product id is listed in SDL_JOYSTICK_BLACKLIST_DEVICES or "
                                 "SDL_GAMECONTROLLER_IGNORE_DEVICES (environment or hint), so SDL deliberately "
                                 "skips it. Remove it from that variable and restart."});
            continue;
        }

        if (seen) {
            if (!sdlGamepads.count(vp)) {
                findings_.push_back({FindingLevel::Info, label(d, known), "Seen as a raw joystick only",
                                     "SDL opened this controller but has no gamepad mapping for it, so it is listed "
                                     "under Joystick with numbered axes and buttons instead of named ones. A mapping "
                                     "can be added with SDL_GAMECONTROLLERCONFIG or SDL_AddGamepadMapping()."});
            }
            continue;
        }

        if (xinput) {
            std::string detail;
            if (msp) {
                detail = std::string("The ") + msp->name + "'s mode switch is set to X (XInput). In that mode its "
                         "receiver presents itself as an Xbox 360-style device (product id " + hex4(msp->xinputPid) +
                         ") using a vendor-specific USB class rather than HID. ";
            } else {
                detail = "This device uses Microsoft's XInput (Xbox 360/One) USB protocol, a vendor-specific USB "
                         "class rather than HID. ";
            }
#if defined(__APPLE__)
            detail += "macOS has no built-in driver for that protocol, so no game-controller interface is ever "
                      "created and SDL cannot see the device at all (SDL can only talk to it directly when built "
                      "with libusb support, which Homebrew's SDL3 is not).";
#elif defined(__linux__)
            {
                std::string drv;
                for (const auto& i : d.interfaces) if (!i.driver.empty()) drv = i.driver;
                if (drv.empty())
                    detail += "No kernel driver is bound to it. The 'xpad' module normally handles this; check "
                              "'lsmod | grep xpad' and load it with 'sudo modprobe xpad'.";
                else
                    detail += "It is bound to the '" + drv + "' driver, but SDL could not open its input node. "
                              "This is usually a permission problem: add yourself to the 'input' group or install "
                              "a udev rule for /dev/input/event*.";
            }
#endif
            if (msp) {
                detail += std::string("\n\nFix: slide the X/D switch on the controller to D. The receiver will "
                                      "re-enumerate as product id ") + hex4(msp->dinputPid) +
                          " (a normal HID gamepad), which SDL picks up immediately.";
            }
            findings_.push_back({FindingLevel::Problem, label(d, known), "In XInput mode: no driver",
                                 std::move(detail)});
            continue;
        }

        // A HID gamepad (or a known mode-switch pad in D mode) that SDL did not list.
        std::string detail = "The OS reports a HID game controller with this id, but SDL did not open it. ";
#if defined(__linux__)
        bool unreadable = false;
        for (const auto& i : d.interfaces)
            for (const auto& node : i.eventNodes)
                if (access(node.c_str(), R_OK) != 0) unreadable = true;
        if (unreadable)
            detail += "Its /dev/input/event* node is not readable by you. Add yourself to the 'input' group "
                      "(then log out and back in) or install a udev rule granting access.";
        else
            detail += "Check SDL_JOYSTICK_HIDAPI and SDL_JOYSTICK_* hints, and whether another program has it "
                      "open exclusively.";
#else
        detail += "Try unplugging and re-plugging the receiver. If it still does not appear, check whether another "
                  "app (e.g. a driver utility such as Logitech G HUB or Steam) is holding it exclusively.";
#endif
        findings_.push_back({FindingLevel::Warning, label(d, known), "Present, not opened by SDL",
                             std::move(detail)});
    }
}

std::string HardwareProbe::usbListing() const {
    std::string lines;
    int count = 0;
    for (const UsbDevice& d : usb_) {
        if (d.cls == 0x09) continue;  // hubs
        ++count;
        char cls[32];
        std::snprintf(cls, sizeof cls, "class %02x/%02x/%02x", d.cls, d.subclass, d.protocol);
        lines += "  " + hex4(d.vendor) + ":" + hex4(d.product) + "  " + cls + "  " +
                 (d.manufacturer.empty() ? "" : d.manufacturer + " ") + d.name + "\n";
    }
    return "USB devices seen by the OS (" + std::to_string(count) + ", hubs omitted):\n" + lines;
}

std::string HardwareProbe::report() const {
    std::string s = "Hardware diagnostics\n" + usbListing();
    if (findings_.empty()) {
        s += "No problems found: every controller the OS sees was enumerated by SDL.\n";
    } else {
        for (const Finding& f : findings_) {
            const char* lvl = f.level == FindingLevel::Problem ? "PROBLEM" : f.level == FindingLevel::Warning ? "WARNING" : "INFO";
            s += std::string("\n[") + lvl + "] " + f.device + ": " + f.summary + "\n" + f.detail + "\n";
        }
    }
    return s;
}
