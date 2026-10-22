#pragma once
#include <SDL.h>
#include <algorithm>
#include <array>
#ifdef __SWITCH__
#include <switch.h>
#endif

namespace starcraft {
// Keep mapped controllers open; disconnect releases all injected state.
struct switch_input {
    SDL_GameController* controller = nullptr;
#ifdef __SWITCH__
    PadState pad{};
    bool pad_ready = false;
#endif
    float x = 640, y = 360;
    bool left = false, right = false, touch = false, quit = false;
    std::array<bool, 11> keys{};
    ~switch_input() { if (controller) SDL_GameControllerClose(controller); }
    static void button(Uint8 button, bool down, int x, int y) {
        SDL_Event e{};
        e.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        e.button.button = button;
        e.button.state = down ? SDL_PRESSED : SDL_RELEASED;
        e.button.x = x; e.button.y = y; e.button.clicks = 1;
        SDL_PushEvent(&e);
    }
    void key(size_t i, bool down, SDL_Scancode scan, SDL_Keycode sym) {
        if (down == keys[i]) return;
        SDL_Event e{};
        e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
        e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
        e.key.keysym.scancode = scan; e.key.keysym.sym = sym;
        SDL_PushEvent(&e); keys[i] = down;
    }
    void update() {
#ifdef __SWITCH__
        if (!pad_ready) { padConfigureInput(1,HidNpadStyleSet_NpadStandard); padInitializeDefault(&pad); hidInitializeTouchScreen(); pad_ready=true; }
        padUpdate(&pad);
        const u64 held=padGetButtons(&pad);
        auto axis = [&](SDL_GameControllerAxis a) {
            auto pos=padGetStickPos(&pad,a==SDL_CONTROLLER_AXIS_RIGHTX || a==SDL_CONTROLLER_AXIS_RIGHTY ? 1 : 0);
            return a==SDL_CONTROLLER_AXIS_RIGHTY || a==SDL_CONTROLLER_AXIS_LEFTY ? -pos.y : pos.x;
        };
        auto pressed = [&](SDL_GameControllerButton b) {
            u64 mask=0;
            switch(b) {
            case SDL_CONTROLLER_BUTTON_A: mask=HidNpadButton_A; break;
            case SDL_CONTROLLER_BUTTON_B: mask=HidNpadButton_B; break;
            case SDL_CONTROLLER_BUTTON_X: mask=HidNpadButton_X; break;
            case SDL_CONTROLLER_BUTTON_Y: mask=HidNpadButton_Y; break;
            case SDL_CONTROLLER_BUTTON_START: mask=HidNpadButton_Plus; break;
            case SDL_CONTROLLER_BUTTON_BACK: mask=HidNpadButton_Minus; break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: mask=HidNpadButton_L; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: mask=HidNpadButton_R; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: mask=HidNpadButton_Left; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: mask=HidNpadButton_Right; break;
            default: break;
            }
            return (held&mask)!=0;
        };
#else
        if (controller && !SDL_GameControllerGetAttached(controller)) { SDL_GameControllerClose(controller); controller=nullptr; }
        if (!controller) for(int i=0;i<SDL_NumJoysticks();++i) if(SDL_IsGameController(i)) { controller=SDL_GameControllerOpen(i); if(controller) break; }
        auto axis = [&](SDL_GameControllerAxis a) { return controller ? SDL_GameControllerGetAxis(controller,a) : 0; };
        auto pressed = [&](SDL_GameControllerButton b) { return controller && SDL_GameControllerGetButton(controller,b); };
#endif
        SDL_Finger* finger = nullptr;
        for (int i = 0; i < SDL_GetNumTouchDevices() && !finger; ++i)
            finger = SDL_GetTouchFinger(SDL_GetTouchDevice(i), 0);
#ifdef __SWITCH__
        HidTouchScreenState touches{};
        SDL_Finger native_finger{};
        if(hidGetTouchScreenStates(&touches,1)>0 && touches.count>0) {
            native_finger.x=touches.touches[0].x/1280.0f;
            native_finger.y=touches.touches[0].y/720.0f;
            finger=&native_finger;
        } else finger=nullptr;
#endif
        touch = finger != nullptr;
        if (touch) { x = finger->x * 1280; y = finger->y * 720; }
        else {
            int rx = axis(SDL_CONTROLLER_AXIS_RIGHTX), ry = axis(SDL_CONTROLLER_AXIS_RIGHTY);
            if (rx > 6000 || rx < -6000) x += rx / 32767.0f * 14;
            if (ry > 6000 || ry < -6000) y += ry / 32767.0f * 14;
        }
        x = std::clamp(x, 0.0f, 1279.0f); y = std::clamp(y, 0.0f, 719.0f);
        bool next_left = touch || pressed(SDL_CONTROLLER_BUTTON_A);
        bool next_right = pressed(SDL_CONTROLLER_BUTTON_B);
        // Motion before release preserves the final drag-selection endpoint.
        SDL_Event motion{}; motion.type = SDL_MOUSEMOTION;
        motion.motion.x = int(x); motion.motion.y = int(y);
        motion.motion.state = (left ? SDL_BUTTON_LMASK : 0) | (right ? SDL_BUTTON_RMASK : 0);
        SDL_PushEvent(&motion);
        if (next_left != left) button(SDL_BUTTON_LEFT, next_left, int(x), int(y));
        if (next_right != right) button(SDL_BUTTON_RIGHT, next_right, int(x), int(y));
        left = next_left; right = next_right;
        int lx = axis(SDL_CONTROLLER_AXIS_LEFTX), ly = axis(SDL_CONTROLLER_AXIS_LEFTY);
        key(0, lx < -15000, SDL_SCANCODE_LEFT, SDLK_LEFT);
        key(1, lx > 15000, SDL_SCANCODE_RIGHT, SDLK_RIGHT);
        key(2, ly < -15000, SDL_SCANCODE_UP, SDLK_UP);
        key(3, ly > 15000, SDL_SCANCODE_DOWN, SDLK_DOWN);
        key(4, pressed(SDL_CONTROLLER_BUTTON_START), SDL_SCANCODE_P, SDLK_p);
        key(5, pressed(SDL_CONTROLLER_BUTTON_LEFTSHOULDER), SDL_SCANCODE_LSHIFT, SDLK_LSHIFT);
        key(6, pressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER), SDL_SCANCODE_RIGHTBRACKET, SDLK_RIGHTBRACKET);
        key(7, pressed(SDL_CONTROLLER_BUTTON_X), SDL_SCANCODE_RETURN, SDLK_RETURN);
        key(8, pressed(SDL_CONTROLLER_BUTTON_Y), SDL_SCANCODE_TAB, SDLK_TAB);
        key(9, pressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT), SDL_SCANCODE_LEFTBRACKET, SDLK_LEFTBRACKET);
        key(10, pressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT), SDL_SCANCODE_RIGHTBRACKET, SDLK_RIGHTBRACKET);
        quit = pressed(SDL_CONTROLLER_BUTTON_BACK);
    }
};
}
