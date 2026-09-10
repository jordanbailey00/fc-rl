#include "fc_api.h"
#include "fc_items.h"
#include "fc_magic.h"
#include "fc_npc.h"
#include "fc_combat.h"
#include "../../fc-core/src/fc_magic_internal.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"loadouts:%d: %s\n",__LINE__,#x); return 1; } } while (0)

static void setup(FcState *s) {
    fc_init(s); fc_reset(s,73);
    fc_set_initial_supplies(s,0,0);
    memset(s->npcs,0,sizeof(s->npcs));
    memset(s->walkable,1,sizeof(s->walkable));
    memset(s->movement_flags,0,sizeof(s->movement_flags));
    memset(s->los_flags,0,sizeof(s->los_flags));
    s->player.x=29; s->player.y=30;
    s->npcs_remaining=1;
    fc_npc_spawn(&s->npcs[0],NPC_TZTOK_JAD,30,30,1);
    s->npcs[0].current_hp=s->npcs[0].max_hp=1000000;
    s->npcs[0].attack_timer=1000000;
}

static int presets(void) {
    FcState s;
    setup(&s);
    CHECK(s.active_loadout==FC_ACTIVE_LOADOUT);
    CHECK(s.player.infinite_resources==(FC_RESOURCE_AMMO|FC_RESOURCE_CHARGES));
    for(int id=FC_LOADOUT_MELEE_LOW;id<FC_NUM_LOADOUTS;id++) {
        setup(&s);
        s.tick=10; s.player.attack_timer=3;
        CHECK(fc_inventory_add(&s,(FcItemStack){385,1,0})==FC_ITEM_OK);
        uint32_t rng=s.rng_state;
        CHECK(fc_apply_loadout(&s,id)==FC_ITEM_OK);
        const FcLoadout *l=&FC_LOADOUTS[id];
        CHECK(s.active_loadout==id && s.tick==10 && s.rng_state==rng && s.player.attack_timer==3);
        CHECK(s.player.inventory[0].item_id==385 && s.player.sharks_remaining==1);
        CHECK(s.player.attack_level==l->attack_lvl && s.player.magic_level==l->magic_lvl);
        CHECK(s.player.strength_level==l->strength_lvl && s.player.defence_level==l->defence_lvl);
        CHECK(s.player.ranged_level==l->ranged_lvl && s.player.prayer_level==l->prayer_lvl);
        CHECK(s.player.max_hp==l->max_hp && s.player.max_prayer==l->max_prayer);
        for(int i=0;i<l->equipment_count;i++) {
            int slot=l->equipment[i].slot, item=(int)l->equipment[i].item_id;
            CHECK(s.player.equipment[slot].item_id==item);
            /* Every prescribed piece can actually be worn at its preset levels. */
            CHECK(fc_unequip_item(&s,slot)==FC_ITEM_OK);
            int inv=0; while(inv<28 && s.player.inventory[inv].item_id!=item) inv++;
            CHECK(inv<28 && fc_equip_item(&s,inv)==FC_ITEM_OK);
        }
        if(l->autocast_spell) {
            CHECK(fc_set_autocast(&s,l->autocast_spell)==FC_MAGIC_OK);
            s.tick=0;
            fc_set_initial_supplies(&s,0,0);
            CHECK(fc_magic_check(&s,l->autocast_spell,1)==FC_MAGIC_OK);
        }
    }
    setup(&s);
    fc_set_initial_supplies(&s,FC_MAX_SHARKS,FC_MAX_PRAYER_DOSES);
    uint32_t before=fc_state_hash(&s);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_MAGIC_LOW)==FC_ITEM_NO_SPACE);
    CHECK(fc_state_hash(&s)==before);
    CHECK(fc_apply_loadout(&s,-1)==FC_ITEM_INVALID && fc_state_hash(&s)==before);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_MELEE_LOW)==FC_ITEM_OK); /* no bag space needed */
    return 0;
}

static int infinite_magic_and_echo(void) {
    FcState s;
    setup(&s);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_MAGIC_MEDIUM)==FC_ITEM_OK);
    FcItemStack inventory[FC_INVENTORY_SLOTS];
    memcpy(inventory,s.player.inventory,sizeof(inventory));
    CHECK(fc_magic_launch(&s,&s.npcs[0],1)==FC_MAGIC_OK);
    CHECK(s.player.attack_timer==6 && s.npcs[0].num_pending_hits==2);
    FcPendingHit *hits=s.npcs[0].pending_hits;
    CHECK(hits[1].ticks_remaining==hits[0].ticks_remaining+1);
    CHECK(hits[1].damage==(hits[0].damage/10*2/5)*10);
    CHECK(s.render_events.player_magic_target_count==2);
    CHECK(s.render_events.player_magic_delay_offset[1]==1);
    CHECK(!memcmp(inventory,s.player.inventory,sizeof(inventory)));
    for(int i=0;i<FC_INVENTORY_SLOTS;i++) if(s.player.inventory[i].item_id==565)
        s.player.inventory[i]=(FcItemStack){0};
    CHECK(fc_magic_check(&s,35,1)==FC_MAGIC_RUNES); /* infinite never bypasses missing runes */
    CHECK(fc_inventory_add_runes(&s)==FC_ITEM_OK);
    CHECK(fc_set_autocast(&s,3)==FC_MAGIC_OK); /* Fire Wave adapts to water weakness */
    fc_npc_spawn(&s.npcs[0],NPC_TZ_KIH,30,30,2);
    CHECK(fc_magic_launch(&s,&s.npcs[0],1)==FC_MAGIC_OK);
    CHECK(s.render_events.player_attack_spell_id==35 && s.player.autocast_spell==3);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_MAGIC_MAX)==FC_ITEM_OK);
    s.npcs[0].num_pending_hits=0;
    CHECK(fc_magic_launch(&s,&s.npcs[0],1)==FC_MAGIC_OK && s.player.attack_timer==4);
    CHECK(s.npcs[0].num_pending_hits==1);
    return 0;
}

static int npc_hits(const FcState *s) {
    int n=0;
    for(int i=0;i<s->render_events.hit_count;i++)
        n+=s->render_events.hits[i].target_entity_type==ENTITY_NPC;
    return n;
}

static int melee_and_ammo(void) {
    int actions[FC_NUM_ACTION_HEADS]={0,1,0,0,0,0,0};
    FcState s;
    setup(&s);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_MELEE_MAX)==FC_ITEM_OK);
    fc_step(&s,actions);
    CHECK(npc_hits(&s)==3 && s.player.attack_timer==4);
    setup(&s);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_MELEE_MAX)==FC_ITEM_OK);
    s.npcs[0].size=1;
    fc_step(&s,actions);
    CHECK(npc_hits(&s)==1);
    setup(&s);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_MELEE_MAX)==FC_ITEM_OK);
    s.npcs[0].size=1;
    fc_npc_spawn(&s.npcs[1],NPC_TZ_KIH,30,29,2);
    fc_npc_spawn(&s.npcs[2],NPC_TZ_KIH,30,31,3);
    s.npcs_remaining=3;
    fc_step(&s,actions);
    CHECK(npc_hits(&s)==3);
    int haste=0, misses=0;
    for(int seed=1;seed<=100;seed++) {
        setup(&s);
        CHECK(fc_apply_loadout(&s,FC_LOADOUT_MELEE_MEDIUM)==FC_ITEM_OK);
        fc_rng_seed(&s,(uint32_t)seed);
        fc_step(&s,actions);
        CHECK(npc_hits(&s)==2);
        CHECK(s.player.attack_timer==2 || s.player.attack_timer==3);
        haste+=s.player.attack_timer==2;
        FcRenderHit *h=s.render_events.hits;
        if(h[0].damage==0) { CHECK(h[1].damage==0); misses++; }
    }
    CHECK(haste>0 && haste<100 && misses>0);
    setup(&s);
    CHECK(fc_apply_loadout(&s,FC_LOADOUT_SOTA_TBOW)==FC_ITEM_OK);
    int ammo=s.player.ammo_count;
    for(int i=0;i<200;i++) fc_step(&s,actions);
    CHECK(s.player.ammo_count==ammo && s.player.equipment[13].quantity==ammo);
    return 0;
}

static int accuracy_reference(void) {
    /* Enumerate two attack rolls against a shared defence roll. */
    for(int a=0;a<12;a++) for(int d=0;d<12;d++) {
        int success=0;
        for(int x=0;x<=a;x++) for(int y=0;y<=a;y++) for(int z=0;z<=d;z++)
            success+=(x>z || y>z);
        float expected=(float)success/((a+1)*(a+1)*(d+1));
        CHECK(fabsf(fc_double_attack_hit_chance(a,d)-expected)<0.000001f);
    }
    return 0;
}

static int weapon_effects(void) {
    int actions[FC_NUM_ACTION_HEADS]={0,1,0,0,0,0,0};
    int ruby=0, diamond=0, fang=0, rerolls=0;
    for(int seed=1;seed<=500;seed++) {
        FcState s;
        setup(&s);
        CHECK(fc_apply_loadout(&s,FC_LOADOUT_MELEE_HIGH)==FC_ITEM_OK);
        fc_rng_seed(&s,(uint32_t)seed);
        int maximum=(320+(s.player.strength_level+8)*(s.player.melee_strength_bonus+64))/640;
        fc_step(&s,actions);
        CHECK(npc_hits(&s)==1);
        int damage=s.render_events.hits[0].damage/10;
        CHECK(!damage || (damage>=maximum*3/20 && damage<=maximum-maximum*3/20));
        fang+=damage>0;

        setup(&s);
        CHECK(fc_apply_loadout(&s,FC_LOADOUT_RANGED_MEDIUM)==FC_ITEM_OK);
        fc_rng_seed(&s,(uint32_t)seed);
        int hp=s.player.current_hp;
        fc_step(&s,actions);
        if(s.player.current_hp<hp) {
            ruby++;
            CHECK(hp-s.player.current_hp==(hp/100)*10);
            CHECK(s.npcs[0].pending_hits[0].damage==1000);
            CHECK(s.ep_no_prayer_hits==0);
        }
        setup(&s);
        CHECK(fc_apply_loadout(&s,FC_LOADOUT_RANGED_LOW)==FC_ITEM_OK);
        fc_rng_seed(&s,(uint32_t)seed);
        maximum=fc_player_ranged_final_max_hit_hp(&s.player,&s.npcs[0]);
        fc_step(&s,actions);
        damage=s.npcs[0].pending_hits[0].damage/10;
        CHECK(damage<=maximum*115/100);
        diamond+=damage>maximum;

        setup(&s);
        CHECK(fc_apply_loadout(&s,FC_LOADOUT_MAGIC_MAX)==FC_ITEM_OK);
        s.player.confliction_missed=1;
        s.player.confliction_spell=s.player.autocast_spell;
        s.player.confliction_target_spawn=s.npcs[0].spawn_index;
        fc_rng_seed(&s,(uint32_t)seed);
        FcState normal=s, other=s;
        normal.player.confliction_missed=0;
        other.player.confliction_target_spawn++;
        CHECK(fc_magic_launch(&s,&s.npcs[0],1)==FC_MAGIC_OK);
        CHECK(fc_magic_launch(&normal,&normal.npcs[0],1)==FC_MAGIC_OK);
        CHECK(fc_magic_launch(&other,&other.npcs[0],1)==FC_MAGIC_OK);
        CHECK(other.npcs[0].pending_hits[0].accurate==normal.npcs[0].pending_hits[0].accurate);
        CHECK(other.npcs[0].pending_hits[0].damage==normal.npcs[0].pending_hits[0].damage);
        CHECK(s.npcs[0].pending_hits[0].accurate>=normal.npcs[0].pending_hits[0].accurate);
        rerolls+=s.npcs[0].pending_hits[0].accurate>normal.npcs[0].pending_hits[0].accurate;
    }
    CHECK(fang>0 && ruby>0 && ruby<100 && diamond>0 && rerolls>0);
    return 0;
}

static int default_resource_preservation(void) {
    FcState infinite, finite;
    fc_init(&infinite); fc_reset(&infinite,73);
    fc_set_initial_supplies(&infinite,0,0);
    finite=infinite;
    finite.player.infinite_resources=0;
    int attacks=0;
    for(int tick=0;tick<20000;tick++) {
        int actions[FC_NUM_ACTION_HEADS]={0,1,1,0,0,0,0};
        actions[0]=tick%7==0 ? 1+tick%16 : 0;
        fc_step(&infinite,actions);
        fc_step(&finite,actions);
        attacks+=infinite.render_events.player_attack_fired;
        FcState normalized=finite;
        normalized.player.infinite_resources=infinite.player.infinite_resources;
        normalized.player.ammo_count=infinite.player.ammo_count;
        normalized.player.equipment[FC_EQUIP_SLOT_AMMO]=infinite.player.equipment[FC_EQUIP_SLOT_AMMO];
        CHECK(fc_state_hash(&normalized)==fc_state_hash(&infinite));
        if(infinite.terminal) {
            fc_reset(&infinite,73+(unsigned)tick);
            fc_set_initial_supplies(&infinite,0,0);
            finite=infinite; finite.player.infinite_resources=0;
        }
    }
    CHECK(attacks>100);
    return 0;
}

int main(void) {
    if(presets() || infinite_magic_and_echo() || melee_and_ammo() || accuracy_reference() ||
       weapon_effects() || default_resource_preservation()) return 1;
    puts("loadouts: gear/levels, atomic replacement, infinite resources, autocast, multihits and accuracy passed");
    return 0;
}
