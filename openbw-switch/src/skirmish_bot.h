#pragma once
#include "game_session.h"
namespace starcraft {
// A small native Terran opponent. This is not Blizzard's campaign AI.
inline void game_session::tick_bot() {
    if(bot_owner<0 || st.current_frame%24) return;
    std::vector<unit_t*> workers, depots, barracks, army;
    unit_t* base=nullptr;
    int supplies=0;
    for(auto& entry:st.player_units[bot_owner]) {
        auto* u=&entry;
        if(unit_dead(u)) continue;
        if(unit_is(u,UnitTypes::Terran_Supply_Depot)) ++supplies;
        if(unit_is(u,UnitTypes::Terran_Command_Center)) { base=u; if(u_completed(u)) depots.push_back(u); }
        if(!u_completed(u)) continue;
        if(ut_worker(u)) workers.push_back(u);
        else if(unit_is(u,UnitTypes::Terran_Barracks)) barracks.push_back(u);
        else if(!ut_building(u) && !ut_turret(u)) army.push_back(u);
    }
    if(!base) return;
    for(auto* cc:depots) if(workers.size()<12 && cc->build_queue.empty()) {
        action_select(bot_owner,cc); action_train(bot_owner,get_unit_type(UnitTypes::Terran_SCV));
    }
    for(auto* b:barracks) if(b->build_queue.empty()) {
        action_select(bot_owner,b); action_train(bot_owner,get_unit_type(UnitTypes::Terran_Marine));
    }
    // Idle workers gather nearby minerals. Building workers retain their orders.
    for(auto* worker:workers) {
        if(worker->order_type->id!=Orders::PlayerGuard && worker->order_type->id!=Orders::Stop) continue;
        unit_t* mineral=nullptr; int best=0x7fffffff;
        for(auto& entry:st.player_units[11]) { auto* m=&entry; if(!unit_is_mineral_field(m) || unit_dead(m)) continue;
            auto d=m->sprite->position-worker->sprite->position;
            int dist=std::abs(d.x)+std::abs(d.y);
            if(dist<best) { mineral=m; best=dist; }
        }
        if(mineral) { action_select(bot_owner,worker); action_default_order(bot_owner,mineral->sprite->position,mineral,nullptr,false); }
    }
    // At most one outstanding construction request; retry if a site is blocked.
    if(st.current_frame-last_bot_build>=1200 && !workers.empty()) {
        int used=st.supply_used[bot_owner][size_t(race_t::terran)].integer_part();
        int avail=st.supply_available[bot_owner][size_t(race_t::terran)].integer_part();
        UnitTypes type=avail-used<5 || !supplies ? UnitTypes::Terran_Supply_Depot : UnitTypes::Terran_Barracks;
        if(type==UnitTypes::Terran_Supply_Depot || barracks.size()<3) {
            auto* t=get_unit_type(type);
            for(auto* worker:workers) {
                if(unit_is_constructing(worker) || worker->order_type->id==Orders::PlaceBuilding || worker->order_type->id==Orders::ConstructingBuilding) continue;
                if(!unit_can_build(worker,t) || !has_available_resources_for(bot_owner,t)) break;
                bool placed=false;
                xy origin=base->sprite->position/32;
                for(int radius=4;radius<18 && !placed;++radius) for(int dy=-radius;dy<=radius && !placed;++dy) for(int dx=-radius;dx<=radius;++dx) {
                    if(std::abs(dx)!=radius && std::abs(dy)!=radius) continue;
                    xy tile=origin+xy(dx,dy); if(tile.x<0 || tile.y<0) continue;
                    xy center=tile*32+t->placement_size/2;
                    if(!is_in_map_bounds(center) || !can_place_building(worker,bot_owner,t,center,false,false)) continue;
                    action_select(bot_owner,worker);
                    placed=action_build(bot_owner,get_order_type(Orders::PlaceBuilding),t,xy_t<size_t>(tile.x,tile.y));
                    if(placed) { last_bot_build=st.current_frame; break; }
                }
                break;
            }
        }
    }
    if(army.size()>=5 && st.current_frame%240==0) {
        unit_t* target=nullptr;
        for(auto& u:st.player_units[command_owner]) if(!unit_dead(&u) && ut_building(&u)) { target=&u; break; }
        if(!target) return;
        for(size_t i=0;i<army.size();i+=12) {
            a_vector<unit_t*> squad;
            for(size_t j=i;j<std::min(i+12,army.size());++j) squad.push_back(army[j]);
            action_select(bot_owner,squad);
            action_order(bot_owner,get_order_type(Orders::AttackMove),target->sprite->position,nullptr,nullptr,false);
        }
    }
}
}
