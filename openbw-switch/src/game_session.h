#pragma once
#include "ui.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace starcraft {
using namespace bwgame;

// The native HUD is intentionally independent of the replay viewer UI.
struct game_session : ui_functions {
    using ui_functions::ui_functions;
    enum class kind { stop, hold, attack, cancel, move, build, train, research, upgrade, page };
    struct command { kind type; int id; std::string name; int minerals = 0, gas = 0; };
    std::vector<command> commands;
    int page = 0, focus = 0, cursor_x = 640, cursor_y = 360;
    int pending_build = -1;
    unit_id builder_id{};
    bool attack_target = false, move_target = false, panel_press = false;
    int bot_owner = -1;
    int last_bot_build = -1200;
    std::string status = "SELECT A UNIT. RIGHT CLICK TO MOVE / GATHER / ATTACK";
    std::string result;

    static const char* unit_name(int id);
    static const char* tech_name(int id);
    static const char* upgrade_name(int id);

    a_vector<unit_t*> own_selection() {
        a_vector<unit_t*> selected;
        for (auto id : current_selection) {
            auto* u = get_unit(id);
            if (u && u->owner == command_owner && !unit_dead(u) && selected.size() < 12) selected.push_back(u);
        }
        action_select(command_owner, selected);
        return selected;
    }
    void refresh_commands() {
        commands = {{kind::stop, 0, "STOP"}, {kind::hold, 0, "HOLD"},
                    {kind::attack, 0, "ATTACK"}, {kind::cancel, 0, "CANCEL"}, {kind::move, 0, "MOVE / GATHER"}};
        auto sel = own_selection();
        if (sel.size() == 1) {
            auto* u = sel.front();
            for (int i = 0; i < 228; ++i) {
                auto* t = get_unit_type(UnitTypes(i));
                if (!unit_can_build(u, t)) continue;
                kind k = ut_worker(u) && ut_building(t) ? kind::build : kind::train;
                commands.push_back({k, i, unit_name(i), t->mineral_cost, t->gas_cost});
            }
            for (int i = 0; i < 44; ++i) {
                auto* t = get_tech_type(TechTypes(i));
                if (unit_can_research(u, t)) commands.push_back({kind::research, i, tech_name(i), t->mineral_cost, t->gas_cost});
            }
            for (int i = 0; i < 61; ++i) {
                auto* t = get_upgrade_type(UpgradeTypes(i));
                if (unit_can_upgrade(u, t, command_owner)) commands.push_back({kind::upgrade, i, upgrade_name(i), upgrade_mineral_cost(command_owner,t), upgrade_gas_cost(command_owner,t)});
            }
        }
        page = std::clamp(page, 0, int((commands.size() - 1) / 8));
        focus = std::clamp(focus, 0, std::min(7, int(commands.size()) - page * 8 - 1));
    }
    void activate(int index) {
        refresh_commands();
        if (index < 0 || index >= int(commands.size())) return;
        const auto cmd = commands[index];
        auto sel = own_selection();
        bool ok = false;
        switch (cmd.type) {
        case kind::stop: ok = action_stop(command_owner, false); break;
        case kind::hold: ok = action_hold_position(command_owner, false); break;
        case kind::move: move_target=true; attack_target=false; pending_build=-1; status="MOVE / GATHER: TAP DESTINATION OR RESOURCE"; return;
        case kind::attack: move_target=false; attack_target = true; pending_build = -1; status = "ATTACK: TAP GROUND OR ENEMY. B TO CANCEL"; return;
        case kind::cancel:
            pending_build = -1; attack_target = false; move_target=false;
            if (sel.size() == 1) {
                auto* u = sel.front();
                if (!u_completed(u)) ok = action_cancel_building_unit(command_owner);
                else if (unit_is_researching(u)) ok = action_cancel_research(command_owner);
                else if (unit_is_upgrading(u)) ok = action_cancel_upgrade(command_owner);
                else ok = action_cancel_build_queue(command_owner, 0);
            }
            break;
        case kind::build:
            if (sel.empty()) return;
            builder_id = get_unit_id(sel.front()); pending_build = cmd.id; attack_target = false; move_target=false;
            status = "BUILD " + cmd.name + ": TAP LOCATION. B TO CANCEL"; return;
        case kind::train:
            if (sel.empty()) return;
            if (ut_addon(get_unit_type(UnitTypes(cmd.id)))) {
                auto* t=get_unit_type(UnitTypes(cmd.id));
                xy tile=(sel.front()->sprite->position-sel.front()->unit_type->placement_size/2+t->addon_position)/32;
                if(tile.x>=0 && tile.y>=0) ok=action_build(command_owner,get_order_type(Orders::PlaceAddon),t,xy_t<size_t>(tile.x,tile.y));
            }
            else if (unit_is_zerg_building(sel.front()) && ut_building(get_unit_type(UnitTypes(cmd.id)))) ok = action_morph_building(command_owner, get_unit_type(UnitTypes(cmd.id)));
            else if (unit_is(sel.front(), UnitTypes::Zerg_Larva) || unit_is(sel.front(), UnitTypes::Zerg_Hydralisk) || unit_is(sel.front(), UnitTypes::Zerg_Mutalisk)) ok = action_morph(command_owner,get_unit_type(UnitTypes(cmd.id)));
            else ok = action_train(command_owner, get_unit_type(UnitTypes(cmd.id)));
            break;
        case kind::research: ok = action_research(command_owner,get_tech_type(TechTypes(cmd.id))); break;
        case kind::upgrade: ok = action_upgrade(command_owner,get_upgrade_type(UpgradeTypes(cmd.id))); break;
        default: break;
        }
        status = ok ? cmd.name + " ACCEPTED" : "CANNOT " + cmd.name + ": CHECK RESOURCES / SUPPLY / SELECTION";
    }
    bool handle_game_event(const native_window::event_t& e) override {
        using E = native_window::event_t;
        if (e.type == E::type_mouse_motion) { cursor_x=e.mouse_x; cursor_y=e.mouse_y; }
        if (e.type == E::type_key_down) {
            if (e.sym == 's') { activate(0); return true; }
            if (e.sym == 'h') { activate(1); return true; }
            if (e.sym == 'a') { activate(2); return true; }
            if (e.sym == 27) { pending_build=-1; attack_target=false; move_target=false; status="CANCELLED"; return true; }
            if (e.sym == '[' || e.sym == ']') { refresh_commands(); page += e.sym == ']' ? 1 : -1; refresh_commands(); return true; }
            if (e.sym == '\t') { refresh_commands(); focus=(focus+1)%std::min(8,int(commands.size())-page*8); return true; }
            if (e.sym == '\r') { activate(page*8+focus); return true; }
        }
        if (e.type == E::type_mouse_button_up && e.button == 1 && panel_press) { panel_press=false; return true; }
        if (e.type != E::type_mouse_button_down) return panel_press && e.type == E::type_mouse_motion;
        cursor_x=e.mouse_x; cursor_y=e.mouse_y;
        if (e.button == 3 && (pending_build >= 0 || attack_target || move_target)) { pending_build=-1; attack_target=false; move_target=false; status="CANCELLED"; return true; }
        if (e.button != 1) return false;
        if (e.mouse_x >= 280 && e.mouse_y >= 550) {
            panel_press=true;
            if (e.mouse_y >= 590 && e.mouse_y < 690) {
                int col=(e.mouse_x-280)/240, row=(e.mouse_y-590)/50;
                if (col < 4) activate(page*8+row*4+col);
            } else if (e.mouse_y >= 690) { refresh_commands(); page=(page+1)%int((commands.size()+7)/8); }
            return true;
        }
        xy pos=screen_pos+xy(e.mouse_x,e.mouse_y);
        if (pending_build >= 0) {
            panel_press=true;
            auto* worker=get_unit(builder_id);
            if (!worker || worker->owner != command_owner || unit_dead(worker)) { pending_build=-1; status="BUILDER LOST"; return true; }
            auto* t=get_unit_type(UnitTypes(pending_build));
            xy tile=(pos-t->placement_size/2)/32;
            if (tile.x<0 || tile.y<0) { status="INVALID LOCATION"; return true; }
            Orders order=unit_race(worker)==race_t::zerg ? Orders::DroneStartBuild : unit_race(worker)==race_t::protoss ? Orders::PlaceProtossBuilding : Orders::PlaceBuilding;
            xy center=tile*32+t->placement_size/2;
            if (!has_available_resources_for(command_owner,t) || !can_place_building(worker,command_owner,t,center,false,false)) { status="CANNOT BUILD HERE / NOT ENOUGH RESOURCES"; return true; }
            action_select(command_owner,worker);
            if (action_build(command_owner,get_order_type(order),t,xy_t<size_t>(tile.x,tile.y))) { pending_build=-1; status="BUILD ORDER ACCEPTED"; }
            return true;
        }
        if (move_target) {
            panel_press=true; own_selection();
            bool ok=action_default_order(command_owner,pos,select_get_unit_at(pos),nullptr,false);
            move_target=false; status=ok?"ORDER ACCEPTED":"CANNOT ISSUE ORDER"; return true;
        }
        if (attack_target) {
            panel_press=true; own_selection();
            bool ok=action_order(command_owner,get_order_type(Orders::AttackMove),pos,select_get_unit_at(pos),nullptr,false);
            attack_target=false; status=ok?"ATTACK ORDER ACCEPTED":"CANNOT ATTACK"; return true;
        }
        return false;
    }
    // Small original 5x7 glyphs, no font or external UI dependency.
    static uint64_t glyph(char c) {
        static const char* chars="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-:/.[+]";
        static const uint64_t bits[]={
            0x1e11111f111111,0x1e11111e11111e,0x0f10101010100f,0x1e11111111111e,
            0x1f10101e10101f,0x1f10101e101010,0x0f10101711110f,0x1111111f111111,
            0x1f04040404041f,0x0101010111110e,0x11121418141211,0x1010101010101f,
            0x111b1515111111,0x11191513111111,0x0e11111111110e,0x1e11111e101010,
            0x0e11111115120d,0x1e11111e141211,0x0f10100e01011e,0x1f040404040404,
            0x1111111111110e,0x11111111110a04,0x11111115151b11,0x11110a040a1111,
            0x11110a04040404,0x1f01020408101f,0x0e11131519110e,0x040c040404040e,
            0x0e11010204081f,0x1e01010e01011e,0x02060a121f0202,0x1f10101e01011e,
            0x0e10101e11110e,0x1f010204080808,0x0e11110e11110e,0x0e11110f01010e,
            0x0000001f000000,0x00040000040000,0x01010204081010,0x00000000000c0c,
            0x0e08080808080e,0x0000040e040000,0x0e02020202020e};
        auto* p=std::strchr(chars,c); return p?bits[p-chars]:0;
    }
    void text(uint8_t* data,size_t pitch,int x,int y,const std::string& str,uint8_t color=255,int scale=2) {
        for (char c:str) {
            auto g=glyph(c);
            for(int row=0;row<7;++row) for(int col=0;col<5;++col) if((g>>((6-row)*8))&(1<<(4-col)))
                fill_rectangle(data,pitch,{{x+col*scale,y+row*scale},{x+(col+1)*scale,y+(row+1)*scale}},color);
            x+=6*scale; if(x+5*scale>=int(screen_width)) break;
        }
    }
    void draw_callback(uint8_t* data,size_t pitch) override {
        refresh_commands();
        fill_rectangle(data,pitch,{{280,550},{1280,720}},0);
        char info[160]; auto r=size_t(st.players[command_owner].race);
        std::snprintf(info,sizeof(info),"MIN %d   GAS %d   SUPPLY %d/%d",st.current_minerals[command_owner],st.current_gas[command_owner],int(st.supply_used[command_owner][r].integer_part()),int(st.supply_available[command_owner][r].integer_part()));
        text(data,pitch,292,558,info);
        for(int j=0;j<8 && page*8+j<int(commands.size());++j) {
            int x=280+j%4*240,y=590+j/4*50; const auto& c=commands[page*8+j];
            line_rectangle(data,pitch,{{x+2,y+2},{x+237,y+47}},j==focus?255:50);
            text(data,pitch,x+8,y+8,c.name.substr(0,18));
            if(c.minerals || c.gas) { std::snprintf(info,sizeof(info),"M %d G %d",c.minerals,c.gas); text(data,pitch,x+8,y+28,info,255,1); }
        }
        text(data,pitch,292,696,"LEFT/RIGHT PAGE  Y NEXT  X ACTIVATE  + PAUSE",255,1);
        text(data,pitch,292,578,status,255,1);
        if(is_paused) text(data,pitch,540,80,"PAUSED");
        if(!result.empty()) text(data,pitch,520,100,result);
        if(pending_build>=0) {
            auto* t=get_unit_type(UnitTypes(pending_build));
            xy tile=(screen_pos+xy(cursor_x,cursor_y)-t->placement_size/2)/32;
            xy from=tile*32-screen_pos;
            line_rectangle(data,pitch,{from,from+t->placement_size},255);
        }
        // A software cursor is necessary: injected motion doesn't warp SDL's OS cursor.
        fill_rectangle(data,pitch,{{cursor_x-7,cursor_y},{cursor_x+8,cursor_y+1}},255);
        fill_rectangle(data,pitch,{{cursor_x,cursor_y-7},{cursor_x+1,cursor_y+8}},255);
    }

    void load_skirmish(const std::string& map_path) {
        auto& u=*this;
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
    bwgame::ui::log("native skirmish: player=%d bot=%d\n",u.command_owner,u.bot_owner);
    }

    void tick_bot();
    void check_result() {
        if(bot_owner<0 || !result.empty()) return;
        int own=0, enemy=0;
        for(auto& u:st.player_units[command_owner]) if(!unit_dead(&u) && ut_building(&u)) ++own;
        for(auto& u:st.player_units[bot_owner]) if(!unit_dead(&u) && ut_building(&u)) ++enemy;
        if(!own || !enemy) { result=own?"VICTORY":"DEFEAT"; is_paused=true; }
    }
};
}
