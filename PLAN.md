# SDL Input Checker — Implementation Plan

A desktop app for macOS and Linux that lists every input device SDL3 can see
(gamepads, raw joysticks, keyboards, mice), lets the user pick one, and draws a
live picture of what that device is reporting.

## 1. Decisions

| Area | Choice | Why |
|---|---|---|
| Language / std | C++20 | `std::span`, designated initialisers, `<format>` where available. |
| Build | CMake ≥ 3.24, single target `sdl-input-checker` | Already installed (4.4.3). 3.24 gives `FetchContent` `FIND_PACKAGE_ARGS`. |
| SDL | SDL3 (≥ 3.2), `find_package(SDL3 CONFIG)` first, `FetchContent` fallback | Homebrew SDL3 3.4.16 is present on this Mac; Linux users without a packaged SDL3 still get a working build. |
| Rendering | `SDL_Renderer` (2D API), vsync on | Sufficient for lines, rects, filled circles; no GL/Metal boilerplate. |
| UI chrome | Dear ImGui via `FetchContent`, with the `imgui_impl_sdl3` + `imgui_impl_sdlrenderer3` backends | Gives the device list, tabs, text, and value read-outs for free. Text rendering without a font library is otherwise the hardest part. |
| Fallback if ImGui is unwanted | `SDL_RenderDebugText` (built into SDL3) | Fixed 8×8 font, good enough for labels; keep the visualiser code independent of ImGui so this swap stays possible. |
| App loop | SDL3 main callbacks (`SDL_AppInit / AppEvent / AppIterate / AppQuit`) | Idiomatic in SDL3 and avoids `main()` pitfalls on macOS. |
| Windowing | One resizable, high-DPI-aware window, logical presentation scaled to a 1280×800 design canvas | Same drawing code on every display; Retina looks correct. |

## 2. Repository layout

```
SDL-input-checker/
├── CMakeLists.txt
├── cmake/
│   └── Dependencies.cmake        # SDL3 + ImGui lookup / fetch
├── src/
│   ├── main.cpp                  # SDL_App* callbacks, wiring only
│   ├── app/
│   │   ├── App.h / App.cpp       # owns window, renderer, registry, current view
│   │   └── Ui.h / Ui.cpp         # ImGui sidebar: device list, selection, status
│   ├── devices/
│   │   ├── DeviceId.h            # {Kind, SDL_JoystickID | SDL_KeyboardID | SDL_MouseID}
│   │   ├── DeviceRegistry.h/.cpp # enumerate, hot-plug, open/close, snapshot state
│   │   ├── GamepadState.h        # buttons, axes, triggers, touchpad, sensors, battery
│   │   ├── JoystickState.h       # raw axes/buttons/hats/balls
│   │   ├── KeyboardState.h       # scancode bitset + last key + text input
│   │   └── MouseState.h          # buttons, abs pos, rel delta history, wheel
│   ├── views/
│   │   ├── View.h                # interface: handleEvent, update(dt), draw(renderer)
│   │   ├── GamepadView.cpp       # controller silhouette with live widgets
│   │   ├── JoystickView.cpp      # generic bars/grid for unmapped devices
│   │   ├── KeyboardView.cpp      # ANSI/ISO keyboard drawing, pressed keys lit
│   │   └── MouseView.cpp         # mouse silhouette, motion trail, wheel meter
│   └── draw/
│       └── Primitives.h/.cpp     # filled circle, rounded rect, arc, thick line, text helpers
├── assets/                       # (optional later) SVG-derived button glyphs
├── .clang-format
└── README.md
```

## 3. CMake plan

- `project(SDLInputChecker LANGUAGES CXX)`, `CMAKE_CXX_STANDARD 20`, `CMAKE_EXPORT_COMPILE_COMMANDS ON`.
- `cmake/Dependencies.cmake`:
  - `FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL GIT_TAG release-3.4.x FIND_PACKAGE_ARGS CONFIG)` → uses Homebrew/apt SDL3 when present, otherwise builds it as a static lib.
  - `FetchContent_Declare(imgui …docking or master tag pinned)`; create an `imgui` static library target from `imgui*.cpp`, `backends/imgui_impl_sdl3.cpp`, `backends/imgui_impl_sdlrenderer3.cpp`, linking `SDL3::SDL3`.
- Target links `SDL3::SDL3 imgui`, warnings `-Wall -Wextra -Wpedantic` (and `/W4` guarded for future MSVC).
- macOS: `MACOSX_BUNDLE` option (off by default) so it can also be run as a plain binary from the terminal; copy an `Info.plist` template when bundling.
- Linux: document the build dependencies SDL3 needs when fetched (`libx11-dev libwayland-dev libxkbcommon-dev libudev-dev libdrm-dev libgbm-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libdbus-1-dev libibus-1.0-dev`).
- CMake presets (`CMakePresets.json`): `debug`, `release`, `asan`.

## 4. Device layer

**Enumeration** (`DeviceRegistry::refresh()`):
- `SDL_GetGamepads()` → entries with `SDL_GetGamepadNameForID`, type, vendor/product, player index, mapping string.
- `SDL_GetJoysticks()` minus IDs that are gamepads → "raw joystick" entries (flight sticks, wheels, unmapped pads).
- `SDL_GetKeyboards()` / `SDL_GetMice()` → named entries. On macOS/Linux these may return an empty list or a single virtual device, so **always add a synthetic "System keyboard" and "System mouse" entry** driven by the global `SDL_GetKeyboardState` / `SDL_GetMouseState`.

**Hot-plug**: react to `SDL_EVENT_GAMEPAD_ADDED/REMOVED`, `SDL_EVENT_JOYSTICK_ADDED/REMOVED`, `SDL_EVENT_KEYBOARD_ADDED/REMOVED`, `SDL_EVENT_MOUSE_ADDED/REMOVED`; if the selected device disappears, fall back to "none" and show a notice.

**Opening**: only the selected gamepad/joystick is opened (`SDL_OpenGamepad` / `SDL_OpenJoystick`); previous one closed on switch. Enable sensors when supported (`SDL_SetGamepadSensorEnabled` for gyro/accel). Keyboards and mice need no open.

**State snapshot per frame** (polled, not event-reconstructed, so the picture is always consistent):
- Gamepad: all `SDL_GamepadButton`s, all `SDL_GamepadAxis` (normalised −1…1, triggers 0…1), touchpad fingers, gyro/accel vectors, battery/power state, connection type, plus a ring buffer of the last N button events for an "event log" panel.
- Joystick: `SDL_GetNumJoystickAxes/Buttons/Hats/Balls` and their values.
- Keyboard: `SDL_GetKeyboardState()` scancode array, modifier state, last keydown with `SDL_GetKeyName` + scancode + keycode, and text-input string when a text field is focused.
- Mouse: buttons mask, absolute window position, accumulated relative motion (from `SDL_EVENT_MOUSE_MOTION` xrel/yrel), wheel deltas with decay, and a short trail of positions.

## 5. Views

Common: each view draws into a fixed 1280×800 logical canvas with the sidebar reserving the left 280 px. Colours: idle grey, pressed accent, analogue values shown both as geometry and as numbers.

- **GamepadView** — the centrepiece. Draw a controller silhouette with:
  - face buttons (labels chosen from `SDL_GetGamepadButtonLabel` so Xbox/PS/Switch names are right),
  - D-pad cross, shoulders, triggers as filling bars, Start/Back/Guide, paddles/misc when reported,
  - two stick wells with the live position dot, deadzone ring, and a fading trail,
  - optional panels: touchpad rectangle with fingers, gyro/accel bars, battery, rumble/LED test buttons (`SDL_RumbleGamepad`, `SDL_SetGamepadLED`),
  - a mapping-info box showing SDL's mapping string and whether it came from the built-in DB, a hint, or the OS.
- **JoystickView** — no assumptions: one horizontal bar per axis, a grid of button squares, one 9-way indicator per hat, ball deltas. This is also the "raw" tab for a gamepad.
- **KeyboardView** — draw a full ANSI layout (rows defined in a small table of `{scancode, x, y, w, h, label}`), highlight pressed keys, show modifier state, a "last key" panel (name, scancode, keycode, repeat flag), and a text-input box to verify IME/dead-key output. Rendering keys by scancode keeps the physical layout correct on non-US keyboards while labels can come from `SDL_GetKeyFromScancode` to reflect the OS layout.
- **MouseView** — mouse silhouette with five buttons lit, wheel meter that decays, an XY pad drawing the motion trail, numeric read-out of position and last delta, and a toggle for `SDL_SetWindowRelativeMouseMode` to test raw/captured motion.

Input focus rule: when a gamepad is selected, disable ImGui gamepad navigation so all pad input reaches the view; keyboard/mouse still drive the sidebar.

## 6. Milestones

1. **Skeleton (½ day)** — CMake, dependency fetch/find, window + renderer, ImGui integration, empty sidebar, "Quit" works on both platforms.
2. **Device registry (½ day)** — enumerate and hot-plug all four kinds, sidebar list with selection, status line with counts. Log to stderr as well.
3. **JoystickView first (½ day)** — generic raw view proves the state/snapshot loop end-to-end for any device.
4. **GamepadView (1–2 days)** — silhouette, sticks, triggers, labels, event log; then sensors, touchpad, rumble/LED.
5. **KeyboardView + MouseView (1 day)** — layout table, highlighting, last-key panel; mouse trail and wheel.
6. **Polish (½–1 day)** — high-DPI check, window resize, colour theme, README with build instructions and screenshots, CMake presets, optional macOS `.app` bundle.

## 7. Verification

- Build in Debug and Release on macOS (Homebrew SDL3) and Linux (packaged SDL3 and fetched SDL3).
- Run with `SDL_LOGGING=all` or `SDL_HINT_JOYSTICK_HIDAPI` toggles to confirm HIDAPI vs OS driver paths.
- Manual matrix: Xbox pad (USB + BT), DualSense (checks touchpad, gyro, LED), Switch Pro (label mapping), a non-mapped joystick, unplug/replug while selected, non-US keyboard layout, external mouse with side buttons.
- ASan preset run through a plug/unplug cycle to catch open/close mistakes.

## 8. Risks and notes

- **Keyboard/mouse enumeration is platform-dependent** in SDL3; the synthetic "System" entries guarantee the views are always usable.
- **Linux permissions**: raw joystick access may need the user in the `input` group or a udev rule; document it.
- **macOS**: game controllers work without extra permissions in a focused SDL window; Bluetooth pads need to be paired first. No Input Monitoring permission is required because we only read events delivered to our own window.
- **ImGui + gamepad**: remember to turn off `ImGuiConfigFlags_NavEnableGamepad` when a pad is being visualised, otherwise the sidebar eats A/B presses.
- `SDL_GetGamepadButtonLabel` and touchpad/sensor APIs exist since 3.2; pin the minimum SDL version accordingly.
- Reference implementation worth reading while building the gamepad drawing: SDL's own `test/testcontroller.c`.
