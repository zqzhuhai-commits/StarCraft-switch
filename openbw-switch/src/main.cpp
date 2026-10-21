// StarCraft 1.16.1 (Brood War) for Nintendo Switch
// Native port using the OpenBW engine (bwgame.h) + SDL2 UI backend.
// Loads game data and maps from SD card: sdmc:/switch/StarCraft/
// Input: right stick / touch -> mouse, A/B -> left/right click, left stick -> camera scroll
#include "ui.h"
#include "common.h"
#include "bwgame.h"
#include "SDL.h"

#include <chrono>
#include <thread>
#include <cstdio>
#include <cstring>

using namespace bwgame;
using ui::log;

FILE* log_file = nullptr;

namespace bwgame {
namespace ui {

void log_str(a_string str) {
	fwrite(str.data(), str.size(), 1, stdout);
	fflush(stdout);
	if (!log_file) log_file = fopen("sdmc:/switch/StarCraft/log.txt", "wb");
	if (log_file) {
		fwrite(str.data(), str.size(), 1, log_file);
		fflush(log_file);
	}
}

void fatal_error_str(a_string str) {
	log("fatal error: %s\n", str);
	SDL_Quit();
	std::terminate();
}

}
}

// ---- SD card map enumeration (libnx fs) ----
#include <switch.h>
#include <vector>
#include <string>

static std::vector<std::string> find_maps_in(const std::string& dir) {
	std::vector<std::string> out;
	FsFileSystem fs;
	if (R_FAILED(fsOpenSdCardFileSystem(&fs))) return out;
	FsDir d;
	if (R_FAILED(fsFsOpenDirectory(&fs, dir.c_str(), FsDirOpenMode_ReadFiles, &d))) {
		fsFsClose(&fs);
		return out;
	}
	while (true) {
		s64 entries = 0;
		FsDirectoryEntry e;
		memset(&e, 0, sizeof(e));
		Result r = fsDirRead(&d, &entries, 1, &e);
		if (R_FAILED(r) || entries == 0) break;
		std::string name = e.name;
		size_t dot = name.rfind('.');
		if (dot != std::string::npos) {
			std::string ext = name.substr(dot);
			if (ext == ".scm" || ext == ".scc" || ext == ".SCM" || ext == ".SCC") {
				out.push_back(dir + "/" + name);
			}
		}
	}
	fsDirClose(&d);
	fsFsClose(&fs);
	return out;
}

// ---- input layer: gamepad/touch -> SDL mouse/keyboard events ----
struct input_state {
	bool have_controller = false;
	float stick_x = 0, stick_y = 0;
	float mouse_x = 1280 / 2, mouse_y = 720 / 2;
	bool a_down = false, b_down = false;
	bool touch_active = false;
	float touch_x = 0, touch_y = 0;
};

static void inject_mouse_move(int x, int y, Uint32 state) {
	SDL_Event e{};
	e.type = SDL_MOUSEMOTION;
	e.motion.x = x;
	e.motion.y = y;
	e.motion.state = state;
	SDL_PushEvent(&e);
}
static void inject_mouse_button(Uint8 button, bool down, int x, int y) {
	SDL_Event e{};
	e.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
	e.button.button = button;
	e.button.x = x;
	e.button.y = y;
	e.button.clicks = 1;
	SDL_PushEvent(&e);
}
static void inject_key(SDL_Keycode sym, SDL_Scancode scancode, bool down) {
	SDL_Event e{};
	e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	e.key.keysym.sym = sym;
	e.key.keysym.scancode = scancode;
	SDL_PushEvent(&e);
}

static void handle_gamepad(input_state& st) {
	if (!st.have_controller) return;
	SDL_GameController* gc = SDL_GameControllerOpen(0);
	if (!gc) return;
	SDL_Joystick* joy = SDL_GameControllerGetJoystick(gc);

	Sint16 rx = SDL_JoystickGetAxis(joy, SDL_CONTROLLER_AXIS_RIGHTX);
	Sint16 ry = SDL_JoystickGetAxis(joy, SDL_CONTROLLER_AXIS_RIGHTY);
	const float deadzone = 6000.0f;
	float dx = 0, dy = 0;
	if (rx > deadzone || rx < -deadzone) dx = (float)rx / 32767.0f;
	if (ry > deadzone || ry < -deadzone) dy = (float)ry / 32767.0f;

	const float mouse_speed = 14.0f;
	st.mouse_x += dx * mouse_speed;
	st.mouse_y += dy * mouse_speed;
	if (st.mouse_x < 0) st.mouse_x = 0;
	if (st.mouse_y < 0) st.mouse_y = 0;
	if (st.mouse_x >= 1280) st.mouse_x = 1279;
	if (st.mouse_y >= 720) st.mouse_y = 719;
	inject_mouse_move((int)st.mouse_x, (int)st.mouse_y, 0);

	bool a = SDL_JoystickGetButton(joy, SDL_CONTROLLER_BUTTON_A) != 0;
	bool b = SDL_JoystickGetButton(joy, SDL_CONTROLLER_BUTTON_B) != 0;
	if (a && !st.a_down) inject_mouse_button(SDL_BUTTON_LEFT, true, (int)st.mouse_x, (int)st.mouse_y);
	if (!a && st.a_down) inject_mouse_button(SDL_BUTTON_LEFT, false, (int)st.mouse_x, (int)st.mouse_y);
	if (b && !st.b_down) inject_mouse_button(SDL_BUTTON_RIGHT, true, (int)st.mouse_x, (int)st.mouse_y);
	if (!b && st.b_down) inject_mouse_button(SDL_BUTTON_RIGHT, false, (int)st.mouse_x, (int)st.mouse_y);
	st.a_down = a;
	st.b_down = b;

	// left stick -> camera scroll (arrow keys, scancodes 79/80/81/82)
	Sint16 lx = SDL_JoystickGetAxis(joy, SDL_CONTROLLER_AXIS_LEFTX);
	Sint16 ly = SDL_JoystickGetAxis(joy, SDL_CONTROLLER_AXIS_LEFTY);
	auto dirkey = [&](SDL_Scancode sc, bool on) {
		static bool prev[4] = {false, false, false, false};
		int idx = sc == SDL_SCANCODE_LEFT ? 0 : sc == SDL_SCANCODE_RIGHT ? 1 : sc == SDL_SCANCODE_DOWN ? 2 : 3;
		if (on && !prev[idx]) inject_key(sc == SDL_SCANCODE_LEFT ? SDLK_LEFT : sc == SDL_SCANCODE_RIGHT ? SDLK_RIGHT : sc == SDL_SCANCODE_DOWN ? SDLK_DOWN : SDLK_UP, sc, true);
		if (!on && prev[idx]) inject_key(sc == SDL_SCANCODE_LEFT ? SDLK_LEFT : sc == SDL_SCANCODE_RIGHT ? SDLK_RIGHT : sc == SDL_SCANCODE_DOWN ? SDLK_DOWN : SDLK_UP, sc, false);
		prev[idx] = on;
	};
	dirkey(SDL_SCANCODE_LEFT, lx < -15000);
	dirkey(SDL_SCANCODE_RIGHT, lx > 15000);
	dirkey(SDL_SCANCODE_UP, ly < -15000);
	dirkey(SDL_SCANCODE_DOWN, ly > 15000);

	SDL_GameControllerClose(gc);
}

static void handle_touch(input_state& st) {
	SDL_TouchID tid = 0;
	int n = SDL_GetNumTouchDevices();
	for (int i = 0; i < n; i++) {
		tid = SDL_GetTouchDevice(i);
		int fingers = SDL_GetNumTouchFingers(tid);
		if (fingers > 0) {
			SDL_Finger* f = SDL_GetTouchFinger(tid, 0);
			if (f) {
				float sx = f->x * 1280.0f;
				float sy = f->y * 720.0f;
				if (!st.touch_active) {
					st.touch_active = true;
					inject_mouse_move((int)sx, (int)sy, SDL_BUTTON_LMASK);
				} else {
					inject_mouse_move((int)sx, (int)sy, SDL_BUTTON_LMASK);
				}
			}
		} else {
			if (st.touch_active) {
				st.touch_active = false;
			}
		}
	}
}

// ---- main ----
int main() {
	size_t screen_width = 1280;
	size_t screen_height = 720;

	log("OpenBW Switch port - boot\n");

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

	// pre-check required game data on SD card
	{
		const char* needed_files[] = {"StarDat.mpq", "BrooDat.mpq", "Patch_rt.mpq"};
		for (const char* f : needed_files) {
			std::string p = std::string("sdmc:/switch/StarCraft/") + f;
			FILE* t = fopen(p.c_str(), "rb");
			if (!t) log("MISSING %s\n", p.c_str());
			else { fclose(t); log("ok    %s\n", p.c_str()); }
		}
	}

	auto load_data_file = data_loading::data_files_directory("sdmc:/switch/StarCraft/");

	game_player player(load_data_file);

	ui_functions ui(std::move(player));
	auto& u = ui;

	u.load_all_image_data(load_data_file);
	u.load_data_file = [&](a_vector<uint8_t>& data, a_string filename) {
		load_data_file(data, std::move(filename));
	};
	u.init();

	auto& wnd = u.wnd;
	if (!wnd.create("StarCraft", 0, 0, screen_width, screen_height)) {
		log("window create failed\n");
		return 1;
	}

	// pick a map from sdmc:/switch/StarCraft/maps/
	auto maps = find_maps_in("sdmc:/switch/StarCraft/maps");
	log("%d maps found\n", (int)maps.size());
	if (maps.empty()) {
		log("no maps found in sdmc:/switch/StarCraft/maps\n");
		return 2;
	}
	std::string map_path = maps[0];
	log("loading map: %s\n", map_path.c_str());

	u.player.load_map_file(map_path);

	u.resize(screen_width, screen_height);
	u.screen_pos = {(int)u.game_st.map_width / 2 - (int)screen_width / 2, (int)u.game_st.map_height / 2 - (int)screen_height / 2};
	u.set_image_data();

	log("game started\n");

	input_state inp;

	auto clock = std::chrono::high_resolution_clock{};
	auto last_tick = clock.now();
	auto tick_speed = std::chrono::milliseconds(42);

	while (true) {
		auto now = clock.now();

		handle_gamepad(inp);
		handle_touch(inp);

		auto tick_t = now - last_tick;
		if (tick_t >= tick_speed * 16) last_tick = now - tick_speed * 16;
		auto tick_n = tick_t / tick_speed;
		for (auto i = tick_n; i;) {
			--i;
			last_tick += tick_speed;
			if (!u.is_done()) u.player.next_frame();
			else break;
		}

		u.update();

		// ~60fps render loop
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}

	return 0;
}
