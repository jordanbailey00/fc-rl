#include "fc_api.h"
#include "fc_items.h"
#include "fc_npc.h"
#include "fc_pathfinding.h"
#include "../../fc-core/src/fc_items_internal.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "equipment: line %d: %s\n", __LINE__, #test); return 1; \
} } while (0)

static void reset(FcState *s, int empty_inventory) {
    fc_init(s);
    fc_reset(s, 101);
    s->player.infinite_resources = 0; /* Finite transaction tests; presets tested separately. */
    if (empty_inventory) fc_set_initial_supplies(s, 0, 0);
}

static int item_slot(const FcPlayer *p, int id) {
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (p->inventory[i].item_id == id) return i;
    return -1;
}

static int loadout_totals(void) {
    /* All existing presets, not only the compiled training preset. This is
     * the real reset helper, not a duplicated test stat calculator. */
    for (int i = 0; i < FC_NUM_LOADOUTS; i++) {
        FcPlayer p = {0};
        const FcLoadout *l = &FC_LOADOUTS[i];
        fc_items_init(&p, l);
        CHECK(p.ranged_attack_bonus == l->ranged_atk);
        CHECK(p.ranged_strength_bonus == l->ranged_str);
        CHECK(p.defence_stab == l->def_stab);
        CHECK(p.defence_slash == l->def_slash);
        CHECK(p.defence_crush == l->def_crush);
        CHECK(p.defence_magic == l->def_magic);
        CHECK(p.defence_ranged == l->def_ranged);
        CHECK(p.prayer_bonus == l->prayer_bonus);
        CHECK(p.weapon_kind == l->weapon_kind);
        CHECK(p.weapon_speed == l->weapon_speed);
        CHECK(p.weapon_range == l->weapon_range);
        CHECK(p.weapon_uses_ammo == l->weapon_uses_ammo);
        CHECK(p.crystal_piece_mask == l->crystal_piece_mask);
        CHECK(p.ammo_count == l->ammo);
    }
    return 0;
}

static int transactions(void) {
    FcState s;
    reset(&s, 0);
    uint32_t before = fc_state_hash(&s);
    CHECK(fc_unequip_item(&s, FC_EQUIP_SLOT_HEAD) == FC_ITEM_NO_SPACE);
    CHECK(fc_state_hash(&s) == before);
    CHECK(fc_equip_item(&s, -1) == FC_ITEM_INVALID);
    CHECK(fc_equip_item(&s, 28) == FC_ITEM_INVALID);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_INVALID); /* food is not equipment */
    CHECK(fc_unequip_item(&s, 6) == FC_ITEM_INVALID); /* client-only arms */
    CHECK(fc_state_hash(&s) == before);
    reset(&s, 1);
    s.player.current_hp = 400;
    s.player.current_prayer = 370;
    s.player.attack_timer = 4;
    s.player.prayer_drain_counter = 21;
    s.player.attack_target_idx = 0;
    CHECK(fc_unequip_item(&s, FC_EQUIP_SLOT_HEAD) == FC_ITEM_OK);
    CHECK(s.player.inventory[0].item_id == 27235);
    CHECK(s.player.ranged_attack_bonus == 203 && s.player.ranged_strength_bonus == 97);
    CHECK(s.player.defence_stab == 108 && s.player.prayer_bonus == 5);
    CHECK(s.player.attack_target_idx == 0);
    uint32_t rng = s.rng_state;
    s.player.defence_level = 79;
    before = fc_state_hash(&s);
    CHECK(fc_equip_item(&s, 0) == FC_ITEM_REQUIREMENTS);
    CHECK(fc_state_hash(&s) == before);
    s.player.defence_level = 99;
    CHECK(fc_equip_item(&s, 0) == FC_ITEM_OK);
    CHECK(s.player.attack_target_idx == -1);
    CHECK(s.player.ranged_attack_bonus == 215 && s.player.ranged_strength_bonus == 99);
    CHECK(s.player.current_hp == 400 && s.player.current_prayer == 370);
    CHECK(s.player.attack_timer == 4 && s.player.prayer_drain_counter == 21);
    CHECK(s.rng_state == rng && s.tick == 0);
    for (int i = 0; i < FC_EQUIPMENT_SLOTS; i++)
        if (s.player.equipment[i].item_id) CHECK(fc_unequip_item(&s, i) == FC_ITEM_OK);
    CHECK(s.player.weapon_kind == FC_WEAPON_UNARMED);
    CHECK(s.player.ranged_attack_bonus == 0 && s.player.defence_stab == 0);
    CHECK(s.player.prayer_bonus == 0 && s.player.ammo_count == 0);
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (s.player.inventory[i].item_id) CHECK(fc_equip_item(&s, i) == FC_ITEM_OK);
    CHECK(s.player.ranged_attack_bonus == 215 && s.player.ranged_strength_bonus == 99);
    CHECK(s.player.equipment[13].quantity == 50000);
    s.terminal = TERMINAL_PLAYER_DEATH;
    before = fc_state_hash(&s);
    CHECK(fc_unequip_item(&s, 0) == FC_ITEM_BUSY);
    CHECK(fc_state_hash(&s) == before);
    return 0;
}

static int two_handed_and_stacks(void) {
    FcState s;
    reset(&s, 0);
    /* Full inventory: a one-for-one shield swap may use the source slot for
     * the two-handed bow when the old shield slot is empty. */
    s.player.inventory[8] = (FcItemStack){12610, 1, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.inventory[8].item_id == 20997);
    CHECK(s.player.equipment[FC_EQUIP_SLOT_SHIELD].item_id == 12610);
    CHECK(s.player.weapon_kind == FC_WEAPON_UNARMED);
    /* With a crossbow AND shield worn, bow needs a second inventory slot. */
    s.player.inventory[9] = (FcItemStack){9185, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    s.player.inventory[9] = (FcItemStack){385, 1, 0};
    uint32_t before = fc_state_hash(&s);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_NO_SPACE);
    CHECK(fc_state_hash(&s) == before);
    s.player.inventory[14] = (FcItemStack){0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.inventory[8].item_id == 9185);
    CHECK(s.player.inventory[14].item_id == 12610);
    CHECK(s.player.equipment[FC_EQUIP_SLOT_SHIELD].item_id == 0);
    reset(&s, 0);
    s.player.inventory[8] = (FcItemStack){11212, 7, 0};
    CHECK(fc_unequip_item(&s, 13) == FC_ITEM_OK); /* merge despite full inventory */
    CHECK(s.player.inventory[8].quantity == 50007);
    CHECK(s.player.ammo_count == 0);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 50007);
    s.player.inventory[8] = (FcItemStack){11212, INT_MAX - 50006, 0};
    before = fc_state_hash(&s);
    CHECK(fc_unequip_item(&s, 13) == FC_ITEM_NO_SPACE); /* whole-stack overflow */
    CHECK(fc_state_hash(&s) == before);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK); /* equip only the portion that fits */
    CHECK(s.player.equipment[13].quantity == INT_MAX);
    CHECK(s.player.inventory[8].quantity == 1);
    before = fc_state_hash(&s);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_NO_SPACE);
    CHECK(fc_state_hash(&s) == before);
    s.player.inventory[8] = (FcItemStack){9143, 100, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 0); /* bolts do not work in a bow */
    CHECK(s.player.inventory[8].item_id == 11212);
    s.player.inventory[9] = (FcItemStack){12788, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK); /* dragon arrows in quiver */
    CHECK(s.player.ammo_count == 0); /* MSB cannot fire dragon arrows */
    s.player.inventory[8] = (FcItemStack){892, 10, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 10);
    s.player.inventory[9] = (FcItemStack){9185, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    s.player.inventory[8] = (FcItemStack){21946, 10, 0};
    CHECK(fc_equip_item(&s, 8) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 0); /* rune crossbow cannot fire dragon bolts */
    s.player.inventory[9] = (FcItemStack){11785, 1, 0};
    CHECK(fc_equip_item(&s, 9) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 10);
    return 0;
}

static int supplies_and_loaded_weapon(void) {
    FcState s;
    reset(&s, 1);
    fc_set_initial_supplies(&s, 2, 5);
    CHECK(s.player.inventory[0].item_id == 2434 && s.player.inventory[1].item_id == 143);
    CHECK(s.player.inventory[2].item_id == 385 && s.player.inventory[3].item_id == 385);
    CHECK(fc_inventory_swap(&s, 1, 27) == FC_ITEM_OK);
    CHECK(fc_select_consumable(&s, 27) == FC_ITEM_OK);
    CHECK(fc_select_consumable(&s, 3) == FC_ITEM_OK);
    s.player.current_hp = 400;
    s.player.current_prayer = 100;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[3] = FC_EAT_SHARK;
    actions[4] = FC_DRINK_PRAYER_POT;
    fc_step(&s, actions);
    CHECK(s.player.inventory[27].item_id == 229);
    CHECK(s.player.inventory[0].item_id == 2434);
    CHECK(s.player.inventory[3].item_id == 0 && s.player.inventory[2].item_id == 385);
    CHECK(s.player.sharks_remaining == 1 && s.player.prayer_doses_remaining == 4);
    CHECK(s.player.selected_food_slot == -1 && s.player.selected_potion_slot == -1);
    reset(&s, 1);
    fc_items_init(&s.player, &FC_LOADOUTS[FC_LOADOUT_BLOWPIPE_PURE]);
    s.player.infinite_resources = 0;
    fc_set_initial_supplies(&s, 0, 0);
    fc_items_spend_ammo(&s.player);
    CHECK(s.player.equipment[3].charges == 49999);
    CHECK(s.player.equipment[13].item_id == 0);
    CHECK(fc_unequip_item(&s, 3) == FC_ITEM_OK);
    CHECK(s.player.inventory[0].charges == 49999);
    CHECK(fc_equip_item(&s, 0) == FC_ITEM_OK);
    CHECK(s.player.ammo_count == 49999);
    return 0;
}

static int combat(void) {
    FcState s;
    reset(&s, 1);
    memset(s.npcs, 0, sizeof(s.npcs));
    memset(s.walkable, 1, sizeof(s.walkable));
    memset(s.movement_flags, 0, sizeof(s.movement_flags));
    memset(s.los_flags, 0, sizeof(s.los_flags));
    s.player.x = 20; s.player.y = 20;
    fc_npc_spawn(&s.npcs[0], NPC_YT_MEJKOT, 20, 25, 1);
    s.npcs_remaining = 1;
    s.npcs[0].attack_timer = 100;
    s.player.attack_target_idx = 0;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    fc_step(&s, actions);
    CHECK(s.render_events.player_attack_fired);
    CHECK(s.player.ammo_count == 49999 && s.player.equipment[13].quantity == 49999);
    FcPendingHit hit = s.npcs[0].pending_hits[0];
    int cooldown = s.player.attack_timer;
    CHECK(fc_unequip_item(&s, 3) == FC_ITEM_OK);
    CHECK(s.player.attack_target_idx == 0 && s.player.attack_timer == cooldown);
    CHECK(memcmp(&hit, &s.npcs[0].pending_hits[0], sizeof(hit)) == 0);
    CHECK(s.player.weapon_range == 1 && s.player.weapon_speed == 4 && !s.player.weapon_uses_ammo);
    s.player.attack_timer = 0;
    s.npcs[0].x = 21; s.npcs[0].y = 21;
    fc_step(&s, actions);
    CHECK(!s.render_events.player_attack_fired); /* diagonal is not melee contact */
    s.npcs[0].x = 21; s.npcs[0].y = 20;
    fc_step(&s, actions);
    CHECK(s.render_events.player_attack_fired);
    CHECK(s.render_events.player_attack_hit_delay_ticks == 1);
    CHECK(s.player.attack_timer == 3 && s.player.ammo_count == 0); /* end-of-tick decrement */
    int melee_hit = 0;
    for (int i = 0; i < s.render_events.hit_count; i++)
        if (s.render_events.hits[i].target_entity_type == ENTITY_NPC &&
            s.render_events.hits[i].attack_style == ATTACK_MELEE) melee_hit = 1;
    CHECK(melee_hit); /* delay-one queues resolve during this tick's hit phase */
    CHECK(fc_inventory_add(&s,(FcItemStack){1432,1,0})==FC_ITEM_OK);
    uint32_t before=fc_state_hash(&s);
    CHECK(fc_equip_item(&s,item_slot(&s.player,1432))==FC_ITEM_REQUIREMENTS);
    CHECK(fc_state_hash(&s)==before);
    s.player.attack_level=40;
    CHECK(fc_equip_item(&s,item_slot(&s.player,1432))==FC_ITEM_OK);
    CHECK(s.player.weapon_kind==FC_WEAPON_MELEE && s.player.weapon_speed==4);
    CHECK(s.player.weapon_range==1 && !s.player.weapon_uses_ammo && s.player.attack_timer==3);
    s.player.attack_timer=0;
    s.player.attack_target_idx=0;
    s.npcs[0].x=21; s.npcs[0].y=21;
    fc_step(&s,actions);
    CHECK(!s.render_events.player_attack_fired);
    s.npcs[0].x=21; s.npcs[0].y=20;
    fc_step(&s,actions);
    CHECK(s.render_events.player_attack_fired && s.player.attack_timer==3);
    melee_hit=0;
    for(int i=0;i<s.render_events.hit_count;i++)
        if(s.render_events.hits[i].target_entity_type==ENTITY_NPC &&
           s.render_events.hits[i].attack_style==ATTACK_MELEE) melee_hit=1;
    CHECK(melee_hit && s.player.ammo_count==0);
    CHECK(fc_equip_item(&s, item_slot(&s.player, 20997)) == FC_ITEM_OK);
    CHECK(s.player.attack_timer == 3 && s.player.attack_target_idx == -1);
    int route_x[64], route_y[64];
    int len = fc_pathfind_attack_position(20, 20, 25, 25, 5, FC_ROUTE_MELEE_RANGE,
        s.walkable, s.movement_flags, s.los_flags, route_x, route_y, 64);
    CHECK(len > 0);
    CHECK(fc_npc_can_melee_player(route_x[len-1], route_y[len-1], 25, 25, 5,
                                s.walkable, s.movement_flags));
    return 0;
}

int main(void) {
    if (loadout_totals() || transactions() || two_handed_and_stacks() ||
        supplies_and_loaded_weapon() || combat()) return 1;
    puts("equipment: preset preservation, atomic transfers, stacks, requirements, supplies and combat passed");
    return 0;
}
