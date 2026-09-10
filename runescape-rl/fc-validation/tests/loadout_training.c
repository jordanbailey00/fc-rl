#include "fight_caves.h"
#include "fc_items.h"
#include "fc_magic.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"training loadout:%d: %s\n",__LINE__,#x); return 1; } } while (0)

/* Compiles unchanged with any FC_ACTIVE_LOADOUT. Exercises the real adapter,
 * not a viewer gear grant, including its reset-time supply configuration. */
int main(void) {
    FightCaves env={0};
    float obs[FC_PUFFER_OBS_SIZE]={0}, actions[FC_PUFFER_NUM_ATNS]={0,1,0};
    float reward=0, terminal=0;
    env.num_agents=1; env.rng=73;
    env.observations=obs; env.actions=actions;
    env.rewards=&reward; env.terminals=&terminal;
    env.reward_params=fc_reward_default_params();
    fc_init(&env.state);
    int attacks=0;
    for(int episode=0;episode<3;episode++) {
        c_reset(&env);
        const FcLoadout *preset=&FC_LOADOUTS[FC_ACTIVE_LOADOUT];
        CHECK(env.state.active_loadout==FC_ACTIVE_LOADOUT);
        CHECK(env.state.player.magic_level==preset->magic_lvl);
        CHECK(env.state.player.attack_level==preset->attack_lvl);
        CHECK(env.state.player.autocast_spell==preset->autocast_spell);
        if(preset->autocast_spell)
            CHECK(fc_magic_check(&env.state,preset->autocast_spell,1)==FC_MAGIC_OK);
        FcItemStack runes[FC_INVENTORY_SLOTS];
        memcpy(runes,env.state.player.inventory,sizeof(runes));
        int ammo=env.state.player.ammo_count;
        for(int tick=0;tick<500 && !terminal;tick++) {
            c_step(&env);
            attacks+=env.state.render_events.player_attack_fired;
            CHECK(env.state.player.ammo_count==ammo);
            CHECK(!memcmp(runes,env.state.player.inventory,sizeof(runes)));
            if(preset->autocast_spell && env.state.render_events.player_attack_fired)
                CHECK(env.state.render_events.player_attack_spell_id==preset->autocast_spell);
        }
    }
    CHECK(attacks>0);
    printf("training loadout: %s, %d attacks through unchanged 3-head adapter\n",
           FC_LOADOUTS[FC_ACTIVE_LOADOUT].name,attacks);
    return 0;
}
