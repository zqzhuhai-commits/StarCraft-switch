// Optional real-data integration test. MPQs/maps are never checked into git.
#include "../openbw-switch/src/game_session.h"
#include "../openbw-switch/src/command_names.h"
#include "../openbw-switch/src/skirmish_bot.h"
#include <SDL.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
namespace bwgame { namespace ui {
void log_str(a_string s) { std::cerr.write(s.data(),s.size()); }
void fatal_error_str(a_string s) { throw std::runtime_error(std::string(s.data(),s.size())); }
}}
int main(int argc,char** argv) {
    if(argc!=3) { std::cerr<<"usage: smoke DATA_DIRECTORY MAP_FILE\n"; return 2; }
    try {
        auto files=bwgame::data_loading::data_files_directory(argv[1]);
        bwgame::game_player player(files);
        starcraft::game_session ui(std::move(player));
        ui.load_data_file=[&](bwgame::a_vector<uint8_t>& data,bwgame::a_string name) { files(data,std::move(name)); };
        std::cerr<<"loading images\n"; ui.init();
        ui.enable_game_commands=true; ui.exit_on_close=false;
        std::cerr<<"loading map\n"; ui.load_skirmish(argv[2]);
        std::cerr<<"map loaded player="<<ui.command_owner<<" bot="<<ui.bot_owner<<"\n";
        SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS,"0");
        ui.resize(1280,720); ui.set_image_data();
        ui.screen_pos=ui.game_st.start_locations[ui.command_owner]-bwgame::xy(640,260);
        // Give the human side the same economy bot only within this test; the
        // production game never automates the human side.
        int human=ui.command_owner, bot=ui.bot_owner;
        for(int frame=0;frame<16000;++frame) {
            ui.tick_bot();
            if(frame<8000) { std::swap(ui.command_owner,ui.bot_owner); ui.tick_bot(); std::swap(ui.command_owner,ui.bot_owner); }
            ui.state_functions::next_frame();
            if(frame%240==0) {
                ui.update();
                int workers=0,marines=0,buildings=0;
                for(auto& u:ui.st.player_units[bot]) {
                    if(ui.unit_is(&u,bwgame::UnitTypes::Terran_SCV)) ++workers;
                    if(ui.unit_is(&u,bwgame::UnitTypes::Terran_Marine)) ++marines;
                    if(ui.ut_building(&u)) ++buildings;
                }
                std::cerr<<"frame="<<ui.st.current_frame<<" minerals="<<ui.st.current_minerals[bot]<<" workers="<<workers<<" marines="<<marines<<" buildings="<<buildings<<"\n";
            }
        }
        int workers=0,marines=0,buildings=0;
        for(auto& u:ui.st.player_units[bot]) {
            if(ui.unit_is(&u,bwgame::UnitTypes::Terran_SCV)) ++workers;
            if(ui.unit_is(&u,bwgame::UnitTypes::Terran_Marine)) ++marines;
            if(ui.ut_building(&u)) ++buildings;
        }
        assert(workers>4 && marines>0 && buildings>1);
        if(const char* snapshot=std::getenv("STARCRAFT_SMOKE_SNAPSHOT")) {
            for(auto& unit:ui.st.player_units[human]) if(ui.ut_worker(&unit) && ui.u_completed(&unit)) { ui.current_selection_add(&unit); break; }
            ui.update();
            auto buffer=ui.get_rgba_buffer();
            auto* pixels=std::get<2>(buffer); int stride=std::get<0>(buffer),height=std::get<1>(buffer);
            std::ofstream image(snapshot,std::ios::binary); image<<"P6\n1280 "<<height<<"\n255\n";
            for(int y=0;y<height;++y) for(int x=0;x<1280;++x) { auto color=pixels[y*stride+x]; for(int b=0;b<3;++b) image.put(char(color>>(b*8))); }
        }
        std::cerr<<"SMOKE PASS frames="<<ui.st.current_frame<<"\n";
    } catch(const std::exception& e) { std::cerr<<"FAILED: "<<e.what()<<"\n"; return 1; }
    SDL_Quit();
}
