#pragma once
#include <SDL3/SDL.h>

#include <string>

// Global keyboard state (SDL3 does not expose a per-device pressed-key
// array), plus the most recent keydown and any text-input composed from it.
struct KeyboardState {
    const bool* pressed = nullptr;  // SDL_GetKeyboardState() array, size SDL_SCANCODE_COUNT
    int numKeys = 0;
    SDL_Keymod mods = SDL_KMOD_NONE;

    SDL_Scancode lastScancode = SDL_SCANCODE_UNKNOWN;
    SDL_Keycode lastKey = SDLK_UNKNOWN;
    bool lastRepeat = false;
    Uint64 lastKeyAtNs = 0;

    std::string textInput;  // accumulated recent SDL_EVENT_TEXT_INPUT, capped

    void sample() {
        pressed = SDL_GetKeyboardState(&numKeys);
        mods = SDL_GetModState();
    }

    void onKeyEvent(const SDL_KeyboardEvent& e) {
        if (!e.down) return;
        lastScancode = e.scancode;
        lastKey = e.key;
        lastRepeat = e.repeat;
        lastKeyAtNs = e.timestamp;
    }

    void onTextInput(const char* text) {
        textInput += text;
        if (textInput.size() > 48) textInput.erase(0, textInput.size() - 48);
    }
};
