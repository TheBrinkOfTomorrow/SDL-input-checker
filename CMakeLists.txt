cmake_minimum_required(VERSION 3.24)
project(SDLInputChecker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SIC_MACOS_BUNDLE "Build a macOS .app bundle instead of a plain executable" OFF)

include(cmake/Dependencies.cmake)

add_executable(sdl-input-checker
    src/main.cpp
    src/app/App.cpp
    src/app/Ui.cpp
    src/devices/DeviceRegistry.cpp
    src/devices/KeyboardLayout.cpp
    src/devices/VirtualJoystick.cpp
    src/draw/Canvas.cpp
    src/views/GamepadView.cpp
    src/views/KeyboardView.cpp
    src/views/MouseView.cpp
    src/views/JoystickView.cpp
)
target_include_directories(sdl-input-checker PRIVATE src)
target_link_libraries(sdl-input-checker PRIVATE SDL3::SDL3 imgui)

if(MSVC)
    target_compile_options(sdl-input-checker PRIVATE /W4)
else()
    target_compile_options(sdl-input-checker PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(APPLE AND SIC_MACOS_BUNDLE)
    # Only for the Info.plist's LSMinimumSystemVersion string below; deliberately
    # not touching CMAKE_OSX_DEPLOYMENT_TARGET itself, which would otherwise
    # mismatch the Homebrew SDL3 dylib's own minimum target and warn at link time.
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        set(SIC_MIN_MACOS "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    else()
        set(SIC_MIN_MACOS "11.0")
    endif()
    set_target_properties(sdl-input-checker PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "SDL Input Checker"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.silvan.sdl-input-checker"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
    )
endif()
