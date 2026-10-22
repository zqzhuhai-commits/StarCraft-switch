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
#include <stdexcept>
#include <algorithm>

using namespace bwgame;
using ui::log;

FILE* log_file = nullptr;

namespace bwgame { namespace ui {
void log_str(a_string str) {
    if (log_file) {
        fwrite(str.data(), 1, str.size(), log_file);
        fflush(log_file);
    }
}
void fatal_error_str(a_string str) {
    log("fatal error: %s\n", str);
    throw std::runtime_error(std::string(str.data(), str.size()));
}
}}

// ---- timing ----
static auto t0 = std::chrono::steady_clock::now();
static double now_sec() {
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

// Use stdio paths consistently. Native Fs paths must not include "sdmc:".
#include "map_files.h"
#include "switch_input.h"
#include "game_session.h"
#include "command_names.h"
#include "skirmish_bot.h"

static bool wait_console(bool choose_map, const std::vector<std::string>& maps, size_t& selected) {
    PadState pad;
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);
    bool redraw = true;
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Plus) return false;
        if (choose_map && !maps.empty()) {
            if (down & HidNpadButton_Down) { selected = (selected + 1) % maps.size(); redraw = true; }
            if (down & HidNpadButton_Up) { selected = (selected + maps.size() - 1) % maps.size(); redraw = true; }
            if (down & HidNpadButton_A) return true;
            if (redraw) {
                consoleClear();
                printf("StarCraft Switch - native 1v1 Terran skirmish\n\n");
                printf("UP/DOWN: map   A: start   +: exit\n\n");
                printf("Fresh Terran bases vs basic native bot. Use standard melee maps.\n");
                printf("Campaign / custom scenario triggers are not loaded.\n\n");
                size_t first = selected / 15 * 15;
                for (size_t i = first; i < maps.size() && i < first + 15; ++i)
                    printf("%s %s\n", i == selected ? ">" : " ", maps[i].c_str());
                printf("\nMap %zu / %zu\n", selected + 1, maps.size());
                redraw = false;
            }
        }
        consoleUpdate(NULL);
        svcSleepThread(16000000);
    }
    return false;
}

// ---- main ----
int run_game() {
	size_t screen_width = 1280;
	size_t screen_height = 720;

	// on-screen text console (visible until SDL initializes)
	consoleInit(NULL);
	printf("\nStarCraft Switch port v0.3.0\n");
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

	log_file = fopen("sdmc:/switch/StarCraft/log.txt", "wb");
    log("boot: fsdev mounted result=%08x\n", (unsigned)fsdev_r);

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
			printf("Missing MPQ data. See switch/StarCraft/log.txt.\nPress + to exit.\n");
            size_t unused = 0;
            wait_console(false, {}, unused);
            consoleExit(NULL);
            return 3;
		}
	}
	auto maps = starcraft::find_maps("sdmc:/switch/StarCraft/maps");
	log("%d maps found in sdmc:/switch/StarCraft/maps\n", (int)maps.size());
	if (maps.empty()) {
		log("ABORT: no .scm/.scc maps found\n");
		printf("No .scm/.scx/.scc maps in switch/StarCraft/maps.\nPress + to exit.\n");
        size_t unused = 0;
        wait_console(false, {}, unused);
        consoleExit(NULL);
        return 2;
	}
	size_t selected = 0;
    if (!wait_console(true, maps, selected)) { consoleExit(NULL); return 0; }
    std::string map_path = maps[selected];
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

	// Only our input bridge synthesizes touch clicks (avoid SDL duplicate events).
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    log("SDL_Init...\n");
	int sdl_r = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	log("SDL_Init done: %d (%s)\n", sdl_r, sdl_r == 0 ? "ok" : SDL_GetError());

	if (sdl_r != 0) { SDL_Quit(); return 1; }

    auto load_data_file = data_loading::data_files_directory("sdmc:/switch/StarCraft/");

	log("creating game_player...\n");
	game_player player(load_data_file);
	log("game_player ok\n");

	starcraft::game_session ui(std::move(player));
	auto& u = ui;

	log("loading image data... (this can take a while)\n");
	u64 loaded_bytes = 0;
	u.load_data_file = [&](a_vector<uint8_t>& data, a_string filename) {
		load_data_file(data, std::move(filename));
		loaded_bytes += data.size();
	};
	// init() loads image data once after initializing sound tables.
	u.init();
	log("image data loaded: %.1f MB in %.1fs\n", (double)loaded_bytes / (1024.0 * 1024.0), now_sec());
    u.exit_on_close = false;
    u.enable_game_commands = true;
	log("ui init done at t=%.1fs\n", now_sec());

	auto& wnd = u.wnd;
	if (!wnd.create("StarCraft", 0, 0, screen_width, screen_height)) {
		log("window create failed at t=%.1fs\n", now_sec());
		return 1;
	}
	log("window created at t=%.1fs\n", now_sec());

    // Native 1v1 skirmish: map terrain/resources, fresh Terran bases, no
    // unsupported campaign triggers or Blizzard AI scripts.
    game_load_functions loader(u.st);
    loader.setup_info.starting_units = 1;
    loader.setup_info.resource_type = 1;
    loader.setup_info.starting_minerals = 50;
    loader.load_map_file(map_path, [&] {
        std::vector<int> slots;
        for (int i=0;i<8;++i) {
            int c=u.st.players[i].controller;
            if(c==player_t::controller_open || c==player_t::controller_computer || c==player_t::controller_occupied || c==player_t::controller_computer_game) slots.push_back(i);
        }
        if(slots.size()<2) throw std::runtime_error("Skirmish needs a map with at least two player slots/start locations");
        u.command_owner=slots[0]; u.bot_owner=slots[1];
        for(int i=0;i<8;++i) {
            u.st.players[i].controller=(i==u.command_owner || i==u.bot_owner) ? player_t::controller_occupied : player_t::controller_closed;
            u.st.players[i].race=race_t::terran;
            u.st.players[i].force=0;
            for(int j=0;j<8;++j) u.st.alliances[i][j]=(i==j);
        }
    });
    if(u.game_st.start_locations[u.command_owner] == u.game_st.start_locations[u.bot_owner])
        throw std::runtime_error("Map has missing or duplicate start locations; choose a standard 1v1 melee map");
    log("native skirmish: player=%d bot=%d\n",u.command_owner,u.bot_owner);
	log("map loaded at t=%.1fs\n", now_sec());

	u.resize(screen_width, screen_height);
	u.screen_pos = u.game_st.start_locations[u.command_owner] - xy(int(screen_width)/2, 260);
	u.set_image_data();

	log("game started at t=%.1fs, entering loop\n", now_sec());

    // Simulate through ui_functions so sound and deselection callbacks stay active.
    starcraft::switch_input input;
    auto deadline = std::chrono::steady_clock::now();
    int frame = 0;
    while (appletMainLoop() && !u.window_closed && !input.quit) {
        SDL_PumpEvents();
        input.update();
        u.update();
        if (!u.is_paused && !u.window_closed && !input.quit) {
            u.tick_bot();
            u.state_functions::next_frame();
            u.check_result();
        }
        if (++frame % 120 == 0) log("t=%.1fs render=%d simulation=%d\n", now_sec(), frame, u.st.current_frame);
        deadline += std::chrono::milliseconds(42);
        auto now = std::chrono::steady_clock::now();
        if (deadline < now) deadline = now;
        std::this_thread::sleep_until(deadline);
    }
    return 0;
}

int main() {
    int result = 0;
    try { result = run_game(); }
    catch (const std::exception& e) {
        if (log_file) { fprintf(log_file, "exception: %s\n", e.what()); fflush(log_file); }
        result = 1;
    }
    SDL_Quit();
    if (log_file) { fclose(log_file); log_file = nullptr; }
    return result;
}
