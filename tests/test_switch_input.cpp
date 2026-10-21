#include <SDL.h>
#include <vector>
#include <cassert>
struct _SDL_GameController {} controller;
static bool connected = true, touching = false;
static int opens = 0, closes = 0;
static Uint8 buttons[SDL_CONTROLLER_BUTTON_MAX]{};
static Sint16 axes[SDL_CONTROLLER_AXIS_MAX]{};
static SDL_Finger finger{1, 0.25f, 0.5f, 1};
static std::vector<SDL_Event> events;
extern "C" {
SDL_bool SDL_GameControllerGetAttached(SDL_GameController*) { return connected ? SDL_TRUE : SDL_FALSE; }
void SDL_GameControllerClose(SDL_GameController*) { ++closes; }
int SDL_NumJoysticks() { return connected ? 1 : 0; }
SDL_bool SDL_IsGameController(int) { return SDL_TRUE; }
SDL_GameController* SDL_GameControllerOpen(int) { ++opens; return &controller; }
Sint16 SDL_GameControllerGetAxis(SDL_GameController*, SDL_GameControllerAxis a) { return axes[a]; }
Uint8 SDL_GameControllerGetButton(SDL_GameController*, SDL_GameControllerButton b) { return buttons[b]; }
int SDL_GetNumTouchDevices() { return 1; }
SDL_TouchID SDL_GetTouchDevice(int) { return 1; }
SDL_Finger* SDL_GetTouchFinger(SDL_TouchID, int) { return touching ? &finger : nullptr; }
int SDL_PushEvent(SDL_Event* e) { events.push_back(*e); return 1; }
}
#include "../openbw-switch/src/switch_input.h"
static bool has(Uint32 type, int button = 0) {
    for (const auto& e : events) if (e.type == type && (!button || e.button.button == button)) return true;
    return false;
}
int main() {
    starcraft::switch_input input;
    input.update(); input.update(); assert(opens == 1);
    events.clear(); buttons[SDL_CONTROLLER_BUTTON_A] = 1; input.update();
    assert(has(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT));
    events.clear(); axes[SDL_CONTROLLER_AXIS_RIGHTX] = 32767; input.update();
    assert(events[0].motion.state & SDL_BUTTON_LMASK); assert(input.x > 640);
    events.clear(); buttons[SDL_CONTROLLER_BUTTON_A] = 0; input.update();
    assert(has(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT));
    events.clear(); touching = true; input.update();
    assert(has(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT)); assert(input.x == 320 && input.y == 360);
    events.clear(); finger.x = 0.5f; input.update();
    assert(events[0].motion.x == 640 && (events[0].motion.state & SDL_BUTTON_LMASK));
    events.clear(); touching = false; input.update(); assert(has(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT));
    buttons[SDL_CONTROLLER_BUTTON_B] = 1; axes[SDL_CONTROLLER_AXIS_LEFTX] = -20000; input.update();
    events.clear(); connected = false; input.update();
    assert(closes == 1); assert(has(SDL_MOUSEBUTTONUP, SDL_BUTTON_RIGHT)); assert(has(SDL_KEYUP));
    assert(!input.left && !input.right && !input.keys[0]);
}
