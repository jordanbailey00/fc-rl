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
    int magic_level, attack_level;
    int magic_attack, magic_damage_permille; /* 30 = +3%, added before rounding */
    int infinite_runes;  /* FC_RUNE_* mask, equipment capability, not a name test */
    int autocast;       /* FC_AUTOCAST_* mask */
    int charge_capacity, depleted_item_id;
    int powered_divisor, powered_multiplier, powered_offset;
    int strength_level, prayer_level;
    int melee_stab, melee_slash; /* Deltas from melee_attack (crush). */
    int melee_type; /* Default weapon stance: 0 crush, 1 stab, 2 slash. */
} FcItemDef;

typedef enum {
    FC_ITEM_OK, FC_ITEM_INVALID, FC_ITEM_NO_SPACE, FC_ITEM_REQUIREMENTS,
    FC_ITEM_BUSY
} FcItemResult;

const FcItemDef *fc_item_definition(int item_id);
int fc_item_count(void);
const FcItemDef *fc_item_at(int index);
const char *fc_item_result_message(FcItemResult result);
/* Immediate inventory transactions between ticks. They do not advance time,
 * reset cooldowns, roll RNG, or alter already-launched attacks. */
FcItemResult fc_equip_item(FcState *state, int inventory_slot);
FcItemResult fc_unequip_item(FcState *state, int equipment_slot);
FcItemResult fc_inventory_swap(FcState *state, int first, int second);
/* Explicit setup/loot insertion. Never called by fc_step or the adapter. */
FcItemResult fc_inventory_add(FcState *state, FcItemStack item);
/* Select the actual slot consumed by the next canonical food/potion action. */
FcItemResult fc_select_consumable(FcState *state, int inventory_slot);
void fc_set_initial_supplies(FcState *state, int sharks, int prayer_doses);
/* Explicit account setup: replace worn gear/levels, preserve inventory, world,
 * RNG and cooldowns. Magic presets add missing runes atomically or fail for space. */
FcItemResult fc_apply_loadout(FcState *state, int loadout_id);
FcItemResult fc_inventory_add_runes(FcState *state);

#endif
