#include "fc_api.h"
#include "fc_magic.h"
#include "fc_items.h"
#include "fc_npc.h"
#include "fc_combat.h"
#include "../../fc-core/src/fc_magic_internal.h"
#include "../../fc-core/src/fc_items_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"magic:%d: %s\n",__LINE__,#x); return 1; } } while (0)
static const int RUNES[] = {554,555,556,557,558,559,560,561,562,565,566,21880};

static void setup(FcState *s) {
    fc_init(s); fc_reset(s,73);
    s->player.infinite_resources = 0; /* Exercise finite rune/charge transactions. */
    fc_set_initial_supplies(s,0,0);
    memset(s->npcs,0,sizeof(s->npcs));
    memset(s->walkable,1,sizeof(s->walkable));
    memset(s->movement_flags,0,sizeof(s->movement_flags));
    memset(s->los_flags,0,sizeof(s->los_flags));
    memset(s->player.equipment,0,sizeof(s->player.equipment));
    s->player.magic_level = s->player.attack_level = s->player.defence_level = 99;
    s->player.x=30; s->player.y=30;
    fc_items_recalculate(&s->player);
    fc_npc_spawn(&s->npcs[0],NPC_TZ_KIH,34,30,1);
    s->npcs_remaining=1;
    for (int i=0;i<12;i++) fc_inventory_add(s,(FcItemStack){RUNES[i],1000,0});
}
static int quantity(const FcPlayer *p,int id) {
    int n=0;
    for(int i=0;i<FC_INVENTORY_SLOTS;i++)
        if(p->inventory[i].item_id==id) n+=p->inventory[i].quantity;
    return n;
}
static int equip(FcState *s,int id,int charges) {
    if(fc_inventory_add(s,(FcItemStack){id,1,charges})!=FC_ITEM_OK) return 0;
    for(int i=0;i<FC_INVENTORY_SLOTS;i++)
        if(s->player.inventory[i].item_id==id) return fc_equip_item(s,i)==FC_ITEM_OK;
    return 0;
}
static int catalogue(void) {
    CHECK(fc_spell_count()==57);
    int books[4]={0},restricted=0;
    for(int i=0;i<fc_spell_count();i++) {
        FcState s; setup(&s);
        const FcSpellDef *spell=fc_spell_at(i);
        CHECK(fc_spell_definition(spell->id)==spell);
        for(int j=0;j<i;j++) CHECK(fc_spell_at(j)->id!=spell->id);
        books[spell->book]++;
        CHECK(fc_set_spellbook(&s,spell->book)==FC_MAGIC_OK);
        if(spell->required_weapon) {
            const FcItemDef *weapon=fc_item_definition(spell->required_weapon);
            CHECK(weapon);
            CHECK(equip(&s,weapon->id,weapon->charge_capacity));
        }
        uint32_t before=fc_state_hash(&s);
        FcMagicResult result=fc_cast_spell(&s,spell->id,0);
        if(spell->target_only) {
            CHECK(result==FC_MAGIC_TARGET); CHECK(before==fc_state_hash(&s));
            restricted++; continue;
        }
        CHECK(result==FC_MAGIC_OK);
        CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
        CHECK(s.npcs[0].num_pending_hits==1);
        CHECK(s.npcs[0].pending_hits[0].spell_id==spell->id);
        CHECK(s.render_events.player_magic_target_count==1);
        CHECK(s.render_events.player_attack_spell_id==spell->id);
        CHECK(s.player.manual_spell==0 && s.player.attack_target_idx==-1);
        CHECK(s.player.attack_timer==5);
        CHECK(s.npcs[0].pending_hits[0].ticks_remaining==4);
    }
    CHECK(books[0]==35 && books[1]==16 && books[2]==0 && books[3]==6);
    CHECK(restricted==4);
    return 0;
}
static int requirements_and_costs(void) {
    FcState s; setup(&s);
    uint32_t before=fc_state_hash(&s);
    CHECK(fc_cast_spell(&s,4,0)==FC_MAGIC_BOOK); CHECK(before==fc_state_hash(&s));
    s.player.magic_level=1;
    before=fc_state_hash(&s);
    CHECK(fc_cast_spell(&s,1,0)==FC_MAGIC_LEVEL); CHECK(before==fc_state_hash(&s));
    s.player.magic_level=99;
    CHECK(fc_set_autocast(&s,1)==FC_MAGIC_WEAPON);
    CHECK(fc_cast_spell(&s,11,0)==FC_MAGIC_WEAPON);
    CHECK(fc_cast_spell(&s,0,0)==FC_MAGIC_INVALID);
    CHECK(fc_cast_spell(&s,1,FC_MAX_NPCS)==FC_MAGIC_TARGET);
    fc_set_initial_supplies(&s,0,0);
    before=fc_state_hash(&s);
    CHECK(fc_cast_spell(&s,1,0)==FC_MAGIC_RUNES); CHECK(before==fc_state_hash(&s));
    CHECK(equip(&s,1387,0)); /* fire staff supplies Fire, not Air/Death */
    CHECK(fc_inventory_add(&s,(FcItemStack){556,4,0})==FC_ITEM_OK);
    CHECK(fc_inventory_add(&s,(FcItemStack){560,1,0})==FC_ITEM_OK);
    CHECK(fc_set_autocast(&s,1)==FC_MAGIC_OK);
    s.npcs[0].num_pending_hits=FC_MAX_PENDING_HITS;
    before=fc_state_hash(&s);
    CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_QUEUE);
    CHECK(before==fc_state_hash(&s));
    s.npcs[0].num_pending_hits=0;
    CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
    CHECK(quantity(&s.player,556)==0 && quantity(&s.player,560)==0);
    CHECK(s.player.autocast_spell==1);
    before=fc_state_hash(&s);
    CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_RUNES);
    CHECK(before==fc_state_hash(&s));
    setup(&s); fc_set_initial_supplies(&s,0,0);
    CHECK(fc_inventory_add(&s,(FcItemStack){4697,5,0})==FC_ITEM_OK); /* smoke: air + fire */
    CHECK(fc_inventory_add(&s,(FcItemStack){560,1,0})==FC_ITEM_OK);
    CHECK(fc_cast_spell(&s,1,0)==FC_MAGIC_OK);
    CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
    CHECK(quantity(&s.player,4697)==0);
    return 0;
}
static int modifiers_and_charges(void) {
    FcState s; setup(&s);
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],1)==16);
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],37)==11); /* Water Strike scales to 8 + floor(8*.4) */
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],11)==25);
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],16)==20);
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],46)==19);
    CHECK(equip(&s,777,0));
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],2)==15);
    s.player.magic_damage_permille=100;
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],1)==17);
    s.player.magic_damage_permille=0;
    CHECK(equip(&s,20714,1));
    CHECK(fc_cast_spell(&s,1,0)==FC_MAGIC_OK);
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],1)==17);
    CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
    CHECK(s.player.equipment[FC_EQUIP_SLOT_SHIELD].charges==0);
    CHECK(s.player.equipment[FC_EQUIP_SLOT_SHIELD].item_id==20716);
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],1)==16);
    setup(&s);
    CHECK(equip(&s,27275,1));
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],0)==34);
    s.player.magic_damage_permille=300;
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],0)==64);
    s.player.magic_damage_permille=800;
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],0)==68);
    s.player.magic_attack_bonus=100;
    CHECK(fc_magic_attack_roll(&s.player,&s.npcs[0],0)==110*364);
    CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
    CHECK(s.player.equipment[FC_EQUIP_SLOT_WEAPON].item_id==27277);
    CHECK(fc_magic_check(&s,0,1)==FC_MAGIC_CHARGES);
    setup(&s);
    CHECK(equip(&s,4675,0));
    CHECK(fc_set_spellbook(&s,FC_BOOK_ANCIENT)==FC_MAGIC_OK);
    CHECK(fc_set_autocast(&s,4)==FC_MAGIC_OK);
    CHECK(fc_unequip_item(&s,FC_EQUIP_SLOT_WEAPON)==FC_ITEM_OK);
    CHECK(s.player.autocast_spell==0);
    return 0;
}
static int impact_effects(void) {
    FcState s; setup(&s);
    FcNpc *n=&s.npcs[0];
    s.tick=100;
    FcPendingHit h={.attack_style=ATTACK_MAGIC,.accurate=1,.spell_id=4};
    fc_magic_resolve(&s,n,&h,0); /* accurate zero can freeze; splash cannot */
    CHECK(n->frozen_until==133 && n->freeze_immune_until==138);
    n->frozen_until=0; n->freeze_immune_until=0; h.accurate=0;
    fc_magic_resolve(&s,n,&h,0); CHECK(n->frozen_until==0);
    h.accurate=1; h.spell_id=58; /* Blood Barrage */
    s.player.current_hp=500;
    fc_magic_resolve(&s,n,&h,190); CHECK(s.player.current_hp==540);
    s.player.current_hp=s.player.max_hp-10;
    fc_magic_resolve(&s,n,&h,190); CHECK(s.player.current_hp==s.player.max_hp);
    h.spell_id=64; fc_magic_resolve(&s,n,&h,0);
    CHECK(n->stat_drain[FC_MAGIC_DRAIN_DEFENCE]==2);
    CHECK(n->stat_restore_tick==200);
    s.tick=200; fc_magic_tick_npc(&s,n);
    CHECK(n->stat_drain[FC_MAGIC_DRAIN_DEFENCE]==1);
    h.spell_id=28; fc_magic_resolve(&s,n,&h,30);
    CHECK(n->poison_severity==6 && n->poison_next_tick==230);
    s.tick=230; fc_magic_tick_npc(&s,n);
    CHECK(n->num_pending_hits==1 && n->pending_hits[0].damage==20);
    CHECK(n->poison_severity==5);
    setup(&s);
    s.player.current_hp=500;
    n=&s.npcs[0]; n->current_hp=90;
    fc_queue_pending_hit(n->pending_hits,&n->num_pending_hits,FC_MAX_PENDING_HITS,300,1,ATTACK_MAGIC,-1,0);
    n->pending_hits[0].spell_id=58; n->pending_hits[0].accurate=1;
    fc_resolve_npc_pending_hits(&s,0);
    CHECK(n->is_dead && s.player.current_hp==520); /* heal actual 9 HP, not rolled 30 */
    return 0;
}
static int area_and_live_steps(void) {
    FcState s; setup(&s);
    fc_npc_spawn(&s.npcs[1],NPC_TZ_KIH,35,30,2);
    fc_npc_spawn(&s.npcs[2],NPC_TZ_KIH,40,30,3);
    s.npcs_remaining=3;
    CHECK(fc_set_spellbook(&s,FC_BOOK_ANCIENT)==FC_MAGIC_OK);
    CHECK(fc_cast_spell(&s,4,0)==FC_MAGIC_OK);
    CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
    CHECK(s.render_events.player_magic_target_count==2);
    CHECK(s.npcs[1].num_pending_hits==1 && s.npcs[2].num_pending_hits==0);
    CHECK(quantity(&s.player,565)==998 && quantity(&s.player,560)==996);
    setup(&s);
    s.npcs[0].current_hp=s.npcs[0].max_hp=100000;
    s.npcs[0].attack_timer=1000;
    CHECK(equip(&s,1387,0));
    CHECK(fc_set_autocast(&s,1)==FC_MAGIC_OK);
    s.player.attack_target_idx=0;
    int actions[FC_NUM_ACTION_HEADS]={0}, launches=0;
    for(int tick=0;tick<11;tick++) {
        fc_step(&s,actions);
        if(s.render_events.player_attack_fired) {
            CHECK(tick==0 || tick==5 || tick==10); launches++;
        }
    }
    CHECK(launches==3);
    CHECK(quantity(&s.player,560)==997);
    fc_reset(&s,73);
    CHECK(s.player.magic_level==1 && s.player.autocast_spell==0 && s.player.manual_spell==0);
    CHECK(!fc_magic_active(&s.player));
    return 0;
}
static int powered_passives_and_levels(void) {
    FcState s; setup(&s);
    CHECK(fc_inventory_add(&s,(FcItemStack){27275,1,2})==FC_ITEM_OK);
    int slot=0;
    while(s.player.inventory[slot].item_id!=27275) slot++;
    s.player.magic_level=84;
    uint32_t before=fc_state_hash(&s);
    CHECK(fc_equip_item(&s,slot)==FC_ITEM_REQUIREMENTS);
    CHECK(fc_state_hash(&s)==before);
    s.player.magic_level=85;
    CHECK(fc_equip_item(&s,slot)==FC_ITEM_OK);
    setup(&s);
    CHECK(equip(&s,22323,20000));
    CHECK(fc_magic_max_hit(&s.player,&s.npcs[0],0)==33);
    s.player.magic_attack_bonus=10000;
    int procs=0;
    for(int i=0;i<100;i++) {
        s.npcs[0].num_pending_hits=0;
        CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
        FcPendingHit hit=s.npcs[0].pending_hits[0];
        if(hit.magic_heal_divisor) {
            procs++;
            CHECK(hit.accurate && hit.damage>=90 && hit.damage<=410);
            s.player.current_hp=500;
            fc_magic_resolve(&s,&s.npcs[0],&hit,hit.damage);
            CHECK(s.player.current_hp==500+(hit.damage/10/2)*10);
        }
    }
    CHECK(procs>0 && procs<100);
    setup(&s);
    CHECK(equip(&s,12899,2500));
    s.player.magic_attack_bonus=10000;
    procs=0;
    for(int i=0;i<100;i++) {
        s.npcs[0].num_pending_hits=0;
        CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
        FcPendingHit hit=s.npcs[0].pending_hits[0];
        if(hit.magic_poison) {
            procs++;
            CHECK(hit.magic_poison==-6);
            fc_magic_resolve(&s,&s.npcs[0],&hit,10);
        }
    }
    CHECK(procs>0 && s.npcs[0].poison_severity==-6);
    for(int tick=30;tick<=270;tick+=30) {
        s.tick=tick;
        s.npcs[0].num_pending_hits=0;
        fc_magic_tick_npc(&s,&s.npcs[0]);
        int damage=6+2*(tick/30-1);
        if(damage>20) damage=20;
        CHECK(s.npcs[0].pending_hits[0].damage==damage*10);
    }
    CHECK(s.npcs[0].poison_severity==-20);
    fc_npc_spawn(&s.npcs[0],NPC_TZTOK_JAD,34,30,2);
    procs=0;
    for(int i=0;i<100;i++) {
        s.npcs[0].num_pending_hits=0;
        CHECK(fc_magic_launch(&s,&s.npcs[0],4)==FC_MAGIC_OK);
        FcPendingHit hit=s.npcs[0].pending_hits[0];
        if(hit.magic_poison) {
            procs++;
            CHECK(hit.magic_poison==6);
            fc_magic_resolve(&s,&s.npcs[0],&hit,10);
        }
    }
    CHECK(procs>0 && s.npcs[0].poison_severity==26);
    return 0;
}
static int tome_rolls_and_launch_snapshot(void) {
    FcState plain, charged; setup(&plain); setup(&charged);
    CHECK(equip(&charged,20714,100));
    plain.player.magic_attack_bonus=charged.player.magic_attack_bonus=100000;
    CHECK(fc_cast_spell(&plain,1,0)==FC_MAGIC_OK);
    CHECK(fc_cast_spell(&charged,1,0)==FC_MAGIC_OK);
    for(unsigned seed=1;seed<=100;seed++) {
        FcState a=plain,b=charged;
        a.rng_state=b.rng_state=seed;
        CHECK(fc_magic_launch(&a,&a.npcs[0],4)==FC_MAGIC_OK);
        CHECK(fc_magic_launch(&b,&b.npcs[0],4)==FC_MAGIC_OK);
        CHECK(a.rng_state==b.rng_state);
        CHECK(b.npcs[0].pending_hits[0].damage==
            (a.npcs[0].pending_hits[0].damage/10*11/10)*10);
    }
    setup(&charged);
    fc_npc_spawn(&charged.npcs[0],NPC_KET_ZEK,34,30,1);
    CHECK(equip(&charged,25574,1));
    charged.player.magic_attack_bonus=100000;
    int base=(charged.player.magic_level+9)*(100000+64);
    CHECK(fc_magic_attack_roll(&charged.player,&charged.npcs[0],64)==base*6/5);
    CHECK(fc_cast_spell(&charged,64,0)==FC_MAGIC_OK);
    CHECK(fc_magic_launch(&charged,&charged.npcs[0],4)==FC_MAGIC_OK);
    CHECK(charged.player.equipment[FC_EQUIP_SLOT_SHIELD].item_id==25576);
    FcPendingHit hit=charged.npcs[0].pending_hits[0];
    CHECK(hit.accurate && hit.magic_curse_boost);
    CHECK(fc_unequip_item(&charged,FC_EQUIP_SLOT_SHIELD)==FC_ITEM_OK);
    fc_magic_resolve(&charged,&charged.npcs[0],&hit,0);
    CHECK(charged.npcs[0].stat_drain[FC_MAGIC_DRAIN_DEFENCE]==36);
    return 0;
}
int main(void) {
    if(catalogue() || requirements_and_costs() || modifiers_and_charges() ||
       impact_effects() || area_and_live_steps() || powered_passives_and_levels() ||
       tome_rolls_and_launch_snapshot()) return 1;
    puts("magic: catalogue, costs, equipment modifiers, charges, effects and live autocast passed");
    return 0;
}
