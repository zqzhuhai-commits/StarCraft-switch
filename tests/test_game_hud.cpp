#include "../openbw-switch/src/game_session.h"
#include "../openbw-switch/src/command_names.h"
#include <cassert>
#include <stdexcept>
namespace bwgame { namespace ui {
void log_str(a_string) {}
void fatal_error_str(a_string s) { throw std::runtime_error(std::string(s.data(),s.size())); }
}}
int main() {
    bwgame::global_state global;
    bwgame::game_state game;
    bwgame::state state;
    state.global=&global; state.game=&game;
    state.players[0].race=bwgame::race_t::terran;
    bwgame::game_player player; player.set_st(state);
    starcraft::game_session ui(std::move(player));
    ui.screen_width=1280; ui.screen_height=720;
    std::vector<uint8_t> pixels(1280*720+32,0x9a);
    ui.draw_callback(pixels.data()+16,1280);
    for(int i=0;i<16;++i) { assert(pixels[i]==0x9a); assert(pixels[1280*720+16+i]==0x9a); }
    // Clipped borders at bottom/right must not touch memory past the image.
    ui.line_rectangle(pixels.data()+16,1280,{{1270,710},{1280,720}},255);
    for(int i=0;i<16;++i) assert(pixels[1280*720+16+i]==0x9a);
    native_window::event_t e;
    e.type=e.type_mouse_button_down; e.button=1; e.mouse_x=780; e.mouse_y=605;
    assert(ui.handle_game_event(e)); assert(ui.attack_target); assert(ui.panel_press);
    e.type=e.type_mouse_button_up; assert(ui.handle_game_event(e)); assert(!ui.panel_press);
    e.type=e.type_mouse_button_down; e.button=3; e.mouse_y=100;
    assert(ui.handle_game_event(e)); assert(!ui.attack_target);
    e.type=e.type_key_down; e.sym='\t'; assert(ui.handle_game_event(e)); assert(ui.focus==1);
    assert(std::string(ui.unit_name(0))=="MARINE");
}
