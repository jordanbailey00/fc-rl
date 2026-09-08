#include "fc_items_internal.h"
#include "fc_api.h"
#include <limits.h>
#include <string.h>

/* Pinned to the existing FcLoadout balance, including legacy d'hide defence
 * and Pegasian strength. Equipment switching must not rebalance training.
 * Only the items supplied by our presets and their consumables are supported. */
enum { AMMO_NONE, AMMO_ARROW, AMMO_BOLT, AMMO_LOADED_DART };
static const FcItemDef ITEMS[] = {
    {.id=1169, .name="Coif", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=2, .ranged_strength=0, .defence={4,6,8,4,4}, .prayer=0,
     .ranged_level=20, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=22109, .name="Ava's assembler", .slot=FC_EQUIP_SLOT_CAPE,
     .ranged_attack=8, .ranged_strength=2, .defence={1,1,1,8,2}, .prayer=0,
     .ranged_level=70, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=19547, .name="Necklace of anguish", .slot=FC_EQUIP_SLOT_NECK,
     .ranged_attack=15, .ranged_strength=5, .defence={0,0,0,0,0}, .prayer=2,
     .ranged_level=0, .defence_level=0, .hitpoints_level=75, .melee_attack=0, .melee_strength=0},
    {.id=27235, .name="Masori mask (f)", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=12, .ranged_strength=2, .defence={8,10,12,12,9}, .prayer=1,
     .ranged_level=80, .defence_level=80, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=27238, .name="Masori body (f)", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=43, .ranged_strength=4, .defence={59,52,64,74,60}, .prayer=1,
     .ranged_level=80, .defence_level=80, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=27241, .name="Masori chaps (f)", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=27, .ranged_strength=2, .defence={35,30,39,46,37}, .prayer=1,
     .ranged_level=80, .defence_level=80, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=26235, .name="Zaryte vambraces", .slot=FC_EQUIP_SLOT_HANDS,
     .ranged_attack=18, .ranged_strength=2, .defence={8,8,8,5,8}, .prayer=1,
     .ranged_level=80, .defence_level=45, .hitpoints_level=0, .melee_attack=-8, .melee_strength=0},
    {.id=13237, .name="Pegasian boots", .slot=FC_EQUIP_SLOT_FEET,
     .ranged_attack=12, .ranged_strength=0, .defence={5,5,5,5,5}, .prayer=0,
     .ranged_level=75, .defence_level=75, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=28310, .name="Venator ring", .slot=FC_EQUIP_SLOT_RING,
     .ranged_attack=10, .ranged_strength=2, .defence={0,0,0,0,0}, .prayer=0,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2503, .name="Black d'hide body", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=30, .ranged_strength=0, .defence={55,47,60,50,55}, .prayer=0,
     .ranged_level=70, .defence_level=40, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2497, .name="Black d'hide chaps", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=17, .ranged_strength=0, .defence={31,25,33,28,31}, .prayer=0,
     .ranged_level=70, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2491, .name="Black d'hide vambraces", .slot=FC_EQUIP_SLOT_HANDS,
     .ranged_attack=11, .ranged_strength=0, .defence={6,5,7,8,0}, .prayer=0,
     .ranged_level=70, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=6328, .name="Snakeskin boots", .slot=FC_EQUIP_SLOT_FEET,
     .ranged_attack=3, .ranged_strength=0, .defence={1,1,2,1,0}, .prayer=0,
     .ranged_level=30, .defence_level=30, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2581, .name="Robin hood hat", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=8, .ranged_strength=0, .defence={4,6,8,4,4}, .prayer=0,
     .ranged_level=40, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=10499, .name="Ava's accumulator", .slot=FC_EQUIP_SLOT_CAPE,
     .ranged_attack=4, .ranged_strength=0, .defence={0,1,0,4,0}, .prayer=0,
     .ranged_level=50, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=1704, .name="Amulet of glory", .slot=FC_EQUIP_SLOT_NECK,
     .ranged_attack=10, .ranged_strength=0, .defence={3,3,3,3,3}, .prayer=3,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=10, .melee_strength=6},
    {.id=12596, .name="Rangers' tunic", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=15, .ranged_strength=0, .defence={6,9,12,6,6}, .prayer=0,
     .ranged_level=40, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=12610, .name="Book of law", .slot=FC_EQUIP_SLOT_SHIELD,
     .ranged_attack=10, .ranged_strength=0, .defence={0,0,0,0,0}, .prayer=5,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2495, .name="Red d'hide chaps", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=14, .ranged_strength=0, .defence={28,22,30,20,28}, .prayer=0,
     .ranged_level=60, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=11126, .name="Combat bracelet", .slot=FC_EQUIP_SLOT_HANDS,
     .ranged_attack=7, .ranged_strength=0, .defence={5,5,5,3,5}, .prayer=0,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=7, .melee_strength=6},
    {.id=2577, .name="Ranger boots", .slot=FC_EQUIP_SLOT_FEET,
     .ranged_attack=8, .ranged_strength=0, .defence={2,3,4,2,0}, .prayer=0,
     .ranged_level=40, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=11826, .name="Armadyl helmet", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=10, .ranged_strength=0, .defence={6,8,10,10,8}, .prayer=1,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=-5, .melee_strength=0},
    {.id=11828, .name="Armadyl chestplate", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=33, .ranged_strength=0, .defence={56,48,61,70,57}, .prayer=1,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=-7, .melee_strength=0},
    {.id=11830, .name="Armadyl chainskirt", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=20, .ranged_strength=0, .defence={32,26,34,40,33}, .prayer=1,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=-6, .melee_strength=0},
    {.id=7462, .name="Barrows gloves", .slot=FC_EQUIP_SLOT_HANDS,
     .ranged_attack=12, .ranged_strength=0, .defence={12,12,12,6,12}, .prayer=0,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=12, .melee_strength=12},
    {.id=23971, .name="Crystal helm", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=9, .ranged_strength=0, .defence={12,8,14,10,18}, .prayer=2,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=0, .melee_strength=0, .crystal_piece=FC_CRYSTAL_PIECE_HELM},
    {.id=23975, .name="Crystal body", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=31, .ranged_strength=0, .defence={46,38,48,44,68}, .prayer=3,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=0, .melee_strength=0, .crystal_piece=FC_CRYSTAL_PIECE_BODY},
    {.id=23979, .name="Crystal legs", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=18, .ranged_strength=0, .defence={26,21,30,34,38}, .prayer=2,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=0, .melee_strength=0, .crystal_piece=FC_CRYSTAL_PIECE_LEGS},
    {.id=9185, .name="Rune crossbow", .slot=FC_EQUIP_SLOT_WEAPON,
     .ranged_attack=90, .ranged_strength=0, .two_handed=0, .ranged_level=61,
     .weapon_kind=0, .speed=5, .range=7, .ammo_kind=AMMO_BOLT, .visual_profile=0},
    {.id=20997, .name="Twisted bow", .slot=FC_EQUIP_SLOT_WEAPON,
     .ranged_attack=70, .ranged_strength=20, .two_handed=1, .ranged_level=85,
     .weapon_kind=1, .speed=5, .range=10, .ammo_kind=AMMO_ARROW, .ammo_tier=1, .visual_profile=1},
    {.id=12788, .name="Magic shortbow (i)", .slot=FC_EQUIP_SLOT_WEAPON,
     .ranged_attack=75, .ranged_strength=0, .two_handed=1, .ranged_level=50,
     .weapon_kind=0, .speed=3, .range=7, .ammo_kind=AMMO_ARROW, .visual_profile=4},
    {.id=12926, .name="Toxic blowpipe", .slot=FC_EQUIP_SLOT_WEAPON,
     .ranged_attack=30, .ranged_strength=20, .two_handed=1, .ranged_level=75,
     .weapon_kind=0, .speed=2, .range=5, .ammo_kind=AMMO_LOADED_DART, .visual_profile=5},
    {.id=11785, .name="Armadyl crossbow", .slot=FC_EQUIP_SLOT_WEAPON,
     .ranged_attack=100, .ranged_strength=0, .two_handed=0, .ranged_level=70,
     .weapon_kind=0, .speed=5, .range=8, .ammo_kind=AMMO_BOLT, .ammo_tier=1, .visual_profile=6, .prayer=1},
    {.id=25867, .name="Bow of faerdhinen (c)", .slot=FC_EQUIP_SLOT_WEAPON,
     .ranged_attack=128, .ranged_strength=106, .two_handed=1, .ranged_level=80,
     .weapon_kind=2, .speed=4, .range=10, .ammo_kind=AMMO_NONE, .visual_profile=7},
    {.id=9143, .name="Adamant bolts", .slot=FC_EQUIP_SLOT_AMMO,
     .stackable=1, .ranged_strength=100, .ammo_kind=AMMO_BOLT},
    {.id=11212, .name="Dragon arrow", .slot=FC_EQUIP_SLOT_AMMO,
     .stackable=1, .ranged_strength=60, .ammo_kind=AMMO_ARROW, .ammo_tier=1},
    {.id=892, .name="Rune arrow", .slot=FC_EQUIP_SLOT_AMMO,
     .stackable=1, .ranged_strength=49, .ammo_kind=AMMO_ARROW},
    {.id=21946, .name="Diamond dragon bolts (e)", .slot=FC_EQUIP_SLOT_AMMO,
     .stackable=1, .ranged_strength=122, .ammo_kind=AMMO_BOLT, .ammo_tier=1},
    {.id=385, .name="Shark", .slot=-1},
    {.id=2434, .name="Prayer potion(4)", .slot=-1},
    {.id=139, .name="Prayer potion(3)", .slot=-1},
    {.id=141, .name="Prayer potion(2)", .slot=-1},
    {.id=143, .name="Prayer potion(1)", .slot=-1},
    {.id=229, .name="Vial", .slot=-1},
};

const FcItemDef *fc_item_definition(int item_id) {
    for (unsigned i = 0; i < sizeof(ITEMS) / sizeof(ITEMS[0]); i++)
        if (ITEMS[i].id == item_id) return &ITEMS[i];
    return NULL;
}

static void recalculate_equipment(FcPlayer *p) {
    p->ranged_attack_bonus = p->ranged_strength_bonus = 0;
    p->defence_stab = p->defence_slash = p->defence_crush = 0;
    p->defence_magic = p->defence_ranged = p->prayer_bonus = 0;
    p->melee_attack_bonus = p->melee_strength_bonus = p->crystal_piece_mask = 0;
    const FcItemDef *weapon = fc_item_definition(p->equipment[FC_EQUIP_SLOT_WEAPON].item_id);
    const FcItemDef *ammo = fc_item_definition(p->equipment[FC_EQUIP_SLOT_AMMO].item_id);
    /* Quiver items may be worn with any weapon, but firing requires both the
     * correct category and a supported tier (RSMod validateArrows/Bolts). */
    int usable_ammo = weapon && ammo && weapon->ammo_kind == ammo->ammo_kind &&
        ammo->ammo_tier <= weapon->ammo_tier;
    for (int i = 0; i < FC_EQUIPMENT_SLOTS; i++) {
        const FcItemDef *item = fc_item_definition(p->equipment[i].item_id);
        if (!item) continue;
        if (i == FC_EQUIP_SLOT_AMMO && !usable_ammo)
            continue;
        p->ranged_attack_bonus += item->ranged_attack;
        p->ranged_strength_bonus += item->ranged_strength;
        p->defence_stab += item->defence[0];
        p->defence_slash += item->defence[1];
        p->defence_crush += item->defence[2];
        p->defence_magic += item->defence[3];
        p->defence_ranged += item->defence[4];
        p->prayer_bonus += item->prayer;
        p->melee_attack_bonus += item->melee_attack;
        p->melee_strength_bonus += item->melee_strength;
        p->crystal_piece_mask |= item->crystal_piece;
    }
    p->weapon_kind = weapon ? weapon->weapon_kind : FC_WEAPON_UNARMED;
    p->weapon_speed = weapon ? weapon->speed : 4;
    p->weapon_range = weapon ? weapon->range : 1;
    p->weapon_uses_ammo = weapon && weapon->ammo_kind != AMMO_NONE;
    p->ammo_count = 0;
    if (weapon && weapon->ammo_kind == AMMO_LOADED_DART) {
        p->ammo_count = p->equipment[FC_EQUIP_SLOT_WEAPON].charges;
        p->ranged_strength_bonus += 17; /* preset's loaded adamant darts */
    } else if (usable_ammo) {
        p->ammo_count = p->equipment[FC_EQUIP_SLOT_AMMO].quantity;
    }
}

static int potion_doses(int id) {
    switch (id) {
        case 2434: return 4;
        case 139: return 3;
        case 141: return 2;
        case 143: return 1;
        default: return 0;
    }
}

static void reset_supplies(FcPlayer *p, int sharks, int doses) {
    if (sharks < 0) sharks = 0;
    if (sharks > FC_MAX_SHARKS) sharks = FC_MAX_SHARKS;
    if (doses < 0) doses = 0;
    if (doses > FC_MAX_PRAYER_DOSES) doses = FC_MAX_PRAYER_DOSES;
    memset(p->inventory, 0, sizeof(p->inventory));
    p->sharks_remaining = sharks;
    p->prayer_doses_remaining = doses;
    const int pots[] = {0, 143, 141, 139, 2434};
    int slot = 0;
    while (doses > 0) {
        int count = doses > 4 ? 4 : doses;
        p->inventory[slot++] = (FcItemStack){pots[count], 1, 0};
        doses -= count;
    }
    for (int i = 0; i < sharks; i++)
        p->inventory[slot++] = (FcItemStack){385, 1, 0};
    p->selected_food_slot = p->selected_potion_slot = -1;
}

void fc_set_initial_supplies(FcState *state, int sharks, int prayer_doses) {
    /* Reset-time configuration only, not a gameplay inventory refill API. */
    if (state && state->tick == 0)
        reset_supplies(&state->player, sharks, prayer_doses);
}

void fc_items_init(FcPlayer *p, const FcLoadout *loadout) {
    memset(p->equipment, 0, sizeof(p->equipment));
    for (int i = 0; i < loadout->equipment_count; i++) {
        const FcLoadoutEquipmentItem *item = &loadout->equipment[i];
        if (item->item_id == 810) continue; /* darts are loaded in the blowpipe */
        p->equipment[item->slot] = (FcItemStack){
            (int)item->item_id, item->slot == FC_EQUIP_SLOT_AMMO ? loadout->ammo : 1,
            item->item_id == 12926 ? loadout->ammo : 0
        };
    }
    recalculate_equipment(p);
    reset_supplies(p, FC_MAX_SHARKS, FC_MAX_PRAYER_DOSES);
}

const char *fc_item_result_message(FcItemResult result) {
    switch (result) {
        case FC_ITEM_OK: return "";
        case FC_ITEM_NO_SPACE: return "You don't have enough inventory space.";
        case FC_ITEM_REQUIREMENTS: return "Your levels are too low to wear this item.";
        case FC_ITEM_BUSY: return "You can't change equipment right now.";
        default: return "You can't use that item here.";
    }
}

static int free_slot(const FcItemStack inventory[FC_INVENTORY_SLOTS]) {
    for (int i = 0; i < FC_INVENTORY_SLOTS; i++)
        if (!inventory[i].item_id) return i;
    return -1;
}

static int add_to_inventory(FcItemStack inventory[FC_INVENTORY_SLOTS], FcItemStack item) {
    if (!item.item_id) return 1;
    const FcItemDef *def = fc_item_definition(item.item_id);
    if (!def || item.quantity <= 0) return 0;
    if (def->stackable) {
        for (int i = 0; i < FC_INVENTORY_SLOTS; i++) {
            if (inventory[i].item_id != item.item_id) continue;
            if (item.quantity > INT_MAX - inventory[i].quantity) return 0;
            inventory[i].quantity += item.quantity;
            return 1;
        }
    }
    int index = free_slot(inventory);
    if (index < 0) return 0;
    inventory[index] = item;
    return 1;
}

static int can_change_items(const FcState *state) {
    return state && !state->terminal && state->player.current_hp > 0;
}

FcItemResult fc_equip_item(FcState *state, int index) {
    if (!can_change_items(state)) return FC_ITEM_BUSY;
    FcPlayer *p = &state->player;
    if (index < 0 || index >= FC_INVENTORY_SLOTS) return FC_ITEM_INVALID;
    FcItemStack incoming = p->inventory[index];
    const FcItemDef *item = fc_item_definition(incoming.item_id);
    if (!item || item->slot < 0 || incoming.quantity <= 0 ||
        (!item->stackable && incoming.quantity != 1)) return FC_ITEM_INVALID;
    if (p->ranged_level < item->ranged_level || p->defence_level < item->defence_level ||
        p->max_hp / 10 < item->hitpoints_level) return FC_ITEM_REQUIREMENTS;
    /* Plan on copies, including both hands. A failed secondary transfer cannot
     * partially equip an item, remove a shield, or interrupt combat. */
    FcItemStack inventory[FC_INVENTORY_SLOTS], equipment[FC_EQUIPMENT_SLOTS];
    memcpy(inventory, p->inventory, sizeof(inventory));
    memcpy(equipment, p->equipment, sizeof(equipment));
    FcItemStack *worn = &equipment[item->slot];
    if (item->stackable && worn->item_id == incoming.item_id) {
        int amount = INT_MAX - worn->quantity;
        if (amount > incoming.quantity) amount = incoming.quantity;
        if (!amount) return FC_ITEM_NO_SPACE;
        worn->quantity += amount;
        inventory[index].quantity -= amount;
        if (!inventory[index].quantity) inventory[index] = (FcItemStack){0};
    } else {
        inventory[index] = *worn;
        *worn = incoming;
    }
    const FcItemDef *weapon = fc_item_definition(equipment[FC_EQUIP_SLOT_WEAPON].item_id);
    int displaced = item->two_handed ? FC_EQUIP_SLOT_SHIELD :
        item->slot == FC_EQUIP_SLOT_SHIELD && weapon && weapon->two_handed ?
        FC_EQUIP_SLOT_WEAPON : -1;
    if (displaced >= 0 && equipment[displaced].item_id) {
        /* RSMod returns the conflict to the source slot when it wasn't needed
         * for a primary swap, otherwise to the first free inventory slot. */
        if (!inventory[index].item_id) inventory[index] = equipment[displaced];
        else if (!add_to_inventory(inventory, equipment[displaced])) return FC_ITEM_NO_SPACE;
        equipment[displaced] = (FcItemStack){0};
    }
    memcpy(p->inventory, inventory, sizeof(inventory));
    memcpy(p->equipment, equipment, sizeof(equipment));
    recalculate_equipment(p);
    p->attack_target_idx = -1; /* held-item action interrupts interaction */
    p->approach_target = 0;
    p->approach_target_x = p->approach_target_y = -1;
    p->approach_target_size = 0;
    return FC_ITEM_OK;
}

FcItemResult fc_unequip_item(FcState *state, int slot) {
    if (!can_change_items(state)) return FC_ITEM_BUSY;
    if (slot < 0 || slot >= FC_EQUIPMENT_SLOTS ||
        !state->player.equipment[slot].item_id) return FC_ITEM_INVALID;
    FcPlayer *p = &state->player;
    FcItemStack inventory[FC_INVENTORY_SLOTS];
    memcpy(inventory, p->inventory, sizeof(inventory));
    if (!add_to_inventory(inventory, p->equipment[slot])) return FC_ITEM_NO_SPACE;
    memcpy(p->inventory, inventory, sizeof(inventory));
    p->equipment[slot] = (FcItemStack){0};
    recalculate_equipment(p);
    /* Worn-item Remove does not cancel the existing interaction. */
    return FC_ITEM_OK;
}

FcItemResult fc_inventory_swap(FcState *state, int first, int second) {
    if (!can_change_items(state)) return FC_ITEM_BUSY;
    if (first < 0 || first >= FC_INVENTORY_SLOTS || second < 0 ||
        second >= FC_INVENTORY_SLOTS) return FC_ITEM_INVALID;
    FcItemStack temp = state->player.inventory[first];
    state->player.inventory[first] = state->player.inventory[second];
    state->player.inventory[second] = temp;
    state->player.selected_food_slot = state->player.selected_potion_slot = -1;
    return FC_ITEM_OK;
}

FcItemResult fc_select_consumable(FcState *state, int slot) {
    if (!can_change_items(state)) return FC_ITEM_BUSY;
    if (slot < 0 || slot >= FC_INVENTORY_SLOTS) return FC_ITEM_INVALID;
    int id = state->player.inventory[slot].item_id;
    if (id == 385) state->player.selected_food_slot = slot;
    else if (potion_doses(id)) state->player.selected_potion_slot = slot;
    else return FC_ITEM_INVALID;
    return FC_ITEM_OK;
}

void fc_items_consume(FcPlayer *p, int potion) {
    int selected = potion ? p->selected_potion_slot : p->selected_food_slot;
    for (int n = -1; n < FC_INVENTORY_SLOTS; n++) {
        int slot = n < 0 ? selected : n;
        if (slot < 0 || slot >= FC_INVENTORY_SLOTS) continue;
        FcItemStack *item = &p->inventory[slot];
        int doses = potion_doses(item->item_id);
        if (potion && doses) {
            const int replacement[] = {229, 143, 141, 139};
            item->item_id = replacement[doses - 1];
            break;
        }
        if (!potion && item->item_id == 385) {
            *item = (FcItemStack){0};
            break;
        }
    }
    if (potion) p->prayer_doses_remaining--;
    else p->sharks_remaining--;
}

void fc_items_spend_ammo(FcPlayer *p) {
    p->ammo_count--;
    FcItemStack *weapon = &p->equipment[FC_EQUIP_SLOT_WEAPON];
    if (weapon->item_id == 12926) weapon->charges--;
    else {
        FcItemStack *ammo = &p->equipment[FC_EQUIP_SLOT_AMMO];
        if (ammo->quantity > 0 && --ammo->quantity == 0) {
            *ammo = (FcItemStack){0};
            recalculate_equipment(p);
        }
    }
}
