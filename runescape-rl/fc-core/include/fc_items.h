#ifndef FC_ITEMS_H
#define FC_ITEMS_H

#include "fc_types.h"
#include "fc_player_init.h"

typedef struct {
    int id;
    const char *name;
    int slot;                    /* -1 for inventory-only items */
    int stackable, two_handed;
    int ranged_level, defence_level, hitpoints_level;
    int ranged_attack, ranged_strength;
    int defence[5];               /* stab, slash, crush, magic, ranged */
    int prayer, melee_attack, melee_strength;
    int weapon_kind, speed, range, ammo_kind;
    int ammo_tier;                /* supported ammunition: 0 standard, 1 dragon */
    int crystal_piece;
    int visual_profile;
} FcItemDef;

typedef enum {
    FC_ITEM_OK, FC_ITEM_INVALID, FC_ITEM_NO_SPACE, FC_ITEM_REQUIREMENTS,
    FC_ITEM_BUSY
} FcItemResult;

const FcItemDef *fc_item_definition(int item_id);
const char *fc_item_result_message(FcItemResult result);
/* Immediate inventory transactions between ticks. They do not advance time,
 * reset cooldowns, roll RNG, or alter already-launched attacks. */
FcItemResult fc_equip_item(FcState *state, int inventory_slot);
FcItemResult fc_unequip_item(FcState *state, int equipment_slot);
FcItemResult fc_inventory_swap(FcState *state, int first, int second);
/* Select the actual slot consumed by the next canonical food/potion action. */
FcItemResult fc_select_consumable(FcState *state, int inventory_slot);
void fc_set_initial_supplies(FcState *state, int sharks, int prayer_doses);

#endif
