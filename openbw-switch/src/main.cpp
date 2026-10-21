// StarCraft 1.16.1 (Brood War) for Nintendo Switch
// Native port using the OpenBW engine (bwgame.h) + SDL2 UI backend.
// Loads game data and maps from SD card: sdmc:/switch/StarCraft/
// Input: right stick / touch -> mouse, A/B -> left/right click, left stick -> camera scroll
#include "ui.h"
#include "common.h"
#include "bwgame.h"
#include "SDL.h"
#include <switch.h>

#include <chrono>
#include <thread>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

using namespace bwgame;
using ui::log;

FILE* log_file = nullptr;

// ---- native fs log writer (independent of fsdev/fopen) ----
static void fslog_raw(const char* s) {
	FsFileSystem fs;
	if (R_FAILED(fsOpenSdCardFileSystem(&fs))) return;
	FsFile f;
	if (R_SUCCEEDED(fsFsOpenFile(&fs, "/switch/StarCraft/log.txt", FsOpenMode_Write | FsOpenMode_Append, &f))) {
		s64 sz = 0;
		fsFileGetSize(&f, &sz);
		u64 written = 0;
		fsFileWrite(&f, sz, s, strlen(s), 0);
		fsFileClose(&f);
	}
	fsFsClose(&fs);
}

namespace bwgame {
namespace ui {

void log_str(a_string str) {
	fwrite(str.data(), str.size(), 1, stdout);
	fflush(stdout);
	// console (visible on screen before SDL takes over)
	fwrite(str.data(), str.size(), 1, stderr);
	fflush(stderr);
	// native fs (always works)
	fslog_raw(str.c_str());
	// fsdev fopen (second channel)
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

// ---- timing ----
static auto t0 = std::chrono::steady_clock::now();
static double now_sec() {
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

// ---- SD card map enumeration (libnx fs) ----
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
				st.touch_active = true;
				inject_mouse_move((int)sx, (int)sy, SDL_BUTTON_LMASK);
			}
		} else {
			st.touch_active = false;
		}
	}
}

// ---- main ----
int main() {
	size_t screen_width = 1280;
	size_t screen_height = 720;

	// on-screen text console (visible until SDL initializes)
	consoleInit(NULL);
	printf("\nStarCraft Switch port v1.4\n");
	printf("t=0.000s boot: console ok\n");
	fflush(stdout);

	// mount SD into stdio so fopen("sdmc:/...") works
	Result fsdev_r = fsdevMountSdmc();
	printf("t=%.3fs fsdevMountSdmc: %08x\n", now_sec(), (unsigned)fsdev_r);

	// make sure sdmc:/switch/StarCraft exists so log.txt can be written
	{
		FsFileSystem fs;
		if (R_SUCCEEDED(fsOpenSdCardFileSystem(&fs))) {
			fsFsCreateDirectory(&fs, "/switch");
			fsFsCreateDirectory(&fs, "/switch/StarCraft");
			fsFsCreateDirectory(&fs, "/switch/StarCraft/maps");
			fsFsClose(&fs);
		}
	}

	log("boot: fsdev mounted\n");

	// ---- data pre-check BEFORE any OpenBW init (missing data = clean abort, no crash) ----
	{
		const char* needed_files[] = {"StarDat.mpq", "BrooDat.mpq", "Patch_rt.mpq"};
		bool ok = true;
		for (const char* f : needed_files) {
			std::string p = std::string("sdmc:/switch/StarCraft/") + f;
			FILE* t = fopen(p.c_str(), "rb");
			if (!t) { log("MISSING %s\n", p.c_str()); ok = false; }
			else { fclose(t); log("ok    %s\n", p.c_str()); }
		}
		if (!ok) {
			log("ABORT: missing game data in sdmc:/switch/StarCraft/\n");
			return 3;
		}
	}
	auto maps = find_maps_in("sdmc:/switch/StarCraft/maps");
	log("%d maps found in sdmc:/switch/StarCraft/maps\n", (int)maps.size());
	if (maps.empty()) {
		log("ABORT: no .scm/.scc maps found\n");
		return 2;
	}
	std::string map_path = maps[0];
	log("will load map: %s\n", map_path.c_str());

	// report mpq sizes (detect stripped/small data packs)
	{
		const char* needed_files[] = {"StarDat.mpq", "BrooDat.mpq", "Patch_rt.mpq"};
		for (const char* f : needed_files) {
			std::string p = std::string("sdmc:/switch/StarCraft/") + f;
			FILE* t = fopen(p.c_str(), "rb");
			if (t) {
				fseek(t, 0, SEEK_END);
				long sz = ftell(t);
				fclose(t);
				log("%s: %.1f MB\n", f, (double)sz / (1024.0 * 1024.0));
			}
		}
	}

	// release the console framebuffer before SDL takes over the screen
	fflush(stdout);
	consoleExit(NULL);

	log("SDL_Init...\n");
	int sdl_r = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	log("SDL_Init done: %d (%s)\n", sdl_r, sdl_r == 0 ? "ok" : SDL_GetError());

	auto load_data_file = data_loading::data_files_directory("sdmc:/switch/StarCraft/");

	log("creating game_player...\n");
	game_player player(load_data_file);
	log("game_player ok\n");

	ui_functions ui(std::move(player));
	auto& u = ui;

	log("loading image data... (this can take a while)\n");
	u64 loaded_bytes = 0;
	u.load_data_file = [&](a_vector<uint8_t>& data, a_string filename) {
		load_data_file(data, std::move(filename));
		loaded_bytes += data.size();
	};
	u.load_all_image_data(load_data_file);
	log("image data loaded: %.1f MB in %.1fs\n", (double)loaded_bytes / (1024.0 * 1024.0), now_sec());
	u.init();
	log("ui init done at t=%.1fs\n", now_sec());

	auto& wnd = u.wnd;
	if (!wnd.create("StarCraft", 0, 0, screen_width, screen_height)) {
		log("window create failed at t=%.1fs\n", now_sec());
		return 1;
	}
	log("window created at t=%.1fs\n", now_sec());

	u.player.load_map_file(map_path);
	log("map loaded at t=%.1fs\n", now_sec());

	u.resize(screen_width, screen_height);
	u.screen_pos = {(int)u.game_st.map_width / 2 - (int)screen_width / 2, (int)u.game_st.map_height / 2 - (int)screen_height / 2};
	u.set_image_data();

	log("game started at t=%.1fs, entering loop\n", now_sec());

	// ---- main loop ----
	input_state st;
	st.have_controller = true;
	int frame = 0;
	u64 last_tick_log = 0;
	while (true) {
		handle_gamepad(st);
		handle_touch(st);

		// OpenBW's update() polls and handles window events internally
		u.update();
		u.wnd.update_surface();

		if (++frame % 120 == 0) {
			log("t=%.1fs frame %d (alive)\n", now_sec(), frame);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(42));
	}

	return 0;
}
