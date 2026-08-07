#ifndef FC_PLAYER_INIT_H
#define FC_PLAYER_INIT_H

#include <stdint.h>

/*
 * fc_player_init.h — Player skill levels, equipment, and consumable config.
 *
 * SINGLE FILE for all player configuration. Change skills, gear,
 * consumables here — both fc-training and fc-viewer use this.
 *
 * Training uses FC_ACTIVE_LOADOUT at compile time. It defaults below and can be
 * overridden by the build system, for example -DFC_ACTIVE_LOADOUT=FC_LOADOUT_BOWFA_CRYSTAL.
 * The viewer can switch loadouts at runtime via the loadout dropdown.
 */

/* ======================================================================== */
/* Loadout selection                                                         */
/*                                                                            */
/* To run training with a different setup, uncomment exactly one active line. */
/* The backend, viewer stats, equipment UI, and player model asset builder    */
/* all read from the loadout table below.                                     */
/* ======================================================================== */

typedef enum {
    FC_LOADOUT_BLACK_DHIDE_RCB = 0,
    FC_LOADOUT_SOTA_TBOW = 1,
    FC_LOADOUT_LOW_DEF_RCB = 2,
    FC_LOADOUT_RCB_PURE = 3,
    FC_LOADOUT_MSBI_PURE = 4,
    FC_LOADOUT_BLOWPIPE_PURE = 5,
    FC_LOADOUT_ACB_ARMADYL = 6,
    FC_LOADOUT_BOWFA_CRYSTAL = 7,
    FC_LOADOUT_TBOW_MASORI = 8,
    FC_LOADOUT_COUNT
} FcLoadoutId;

#ifndef FC_ACTIVE_LOADOUT
#define FC_ACTIVE_LOADOUT FC_LOADOUT_SOTA_TBOW
#endif

/* ======================================================================== */
/* Loadout definitions                                                       */
/* ======================================================================== */

#define FC_LOADOUT_EQUIP_MAX 12
#define FC_LOADOUT_MODEL_ITEM_MAX 12
#define FC_PLAYER_MODEL_BASE 0xFC000000u

typedef enum {
    FC_EQUIP_SLOT_HEAD = 0,
    FC_EQUIP_SLOT_CAPE = 1,
    FC_EQUIP_SLOT_NECK = 2,
    FC_EQUIP_SLOT_WEAPON = 3,
    FC_EQUIP_SLOT_BODY = 4,
    FC_EQUIP_SLOT_SHIELD = 5,
    FC_EQUIP_SLOT_AMMO = 6,
    FC_EQUIP_SLOT_LEGS = 7,
    FC_EQUIP_SLOT_HANDS = 9,
    FC_EQUIP_SLOT_FEET = 10,
    FC_EQUIP_SLOT_RING = 12,
} FcEquipmentSlot;

typedef struct {
    int slot;
    uint32_t item_id;       /* Item ID shown in the equipment UI. */
    uint32_t icon_item_id;  /* Optional alternate icon; 0 means item_id. */
    const char* label;
} FcLoadoutEquipmentItem;

typedef enum {
    FC_CRYSTAL_PIECE_NONE = 0,
    FC_CRYSTAL_PIECE_HELM = 1 << 0,
    FC_CRYSTAL_PIECE_BODY = 1 << 1,
    FC_CRYSTAL_PIECE_LEGS = 1 << 2,
    FC_CRYSTAL_PIECE_ALL = FC_CRYSTAL_PIECE_HELM |
                           FC_CRYSTAL_PIECE_BODY |
                           FC_CRYSTAL_PIECE_LEGS
} FcCrystalPieceMask;

/* Exact per-piece modifiers. One percentage point is 100 basis points. */
#define FC_CRYSTAL_HELM_ACCURACY_BP  500
#define FC_CRYSTAL_HELM_DAMAGE_BP    250
#define FC_CRYSTAL_BODY_ACCURACY_BP 1500
#define FC_CRYSTAL_BODY_DAMAGE_BP    750
#define FC_CRYSTAL_LEGS_ACCURACY_BP 1000
#define FC_CRYSTAL_LEGS_DAMAGE_BP    500

typedef struct {
    const char* name;
    const char* weapon_name;
    uint32_t player_model_id;
    int combat_style_profile;
    int max_hp, max_prayer;
    int attack_lvl, strength_lvl, defence_lvl;
    int ranged_lvl, prayer_lvl, magic_lvl;
    int weapon_kind;
    int weapon_uses_ammo;
    int crystal_piece_mask;
    int weapon_speed;
    int weapon_range;
    int ranged_atk, ranged_str;
    int def_stab, def_slash, def_crush, def_magic, def_ranged;
    int prayer_bonus;
    int ammo;
    int equipment_count;
    FcLoadoutEquipmentItem equipment[FC_LOADOUT_EQUIP_MAX];
    int model_item_count;
    int model_item_ids[FC_LOADOUT_MODEL_ITEM_MAX];
} FcLoadout;

typedef enum {
    FC_WEAPON_GENERIC_RANGED = 0,
    FC_WEAPON_TWISTED_BOW = 1,
    FC_WEAPON_BOW_OF_FAERDHINEN = 2
} FcWeaponKind;

/*
 * LOADOUT A: Mid-level — Black D'hide + Rune Crossbow
 *
 *   Slot        Item                    Rng Atk  Rng Str  Stab  Slash Crush Magic Ranged Prayer
 *   ----        ----                    -------  -------  ----  ----- ----- ----- ------ ------
 *   Head        Coif                      2        0        4     6     8     4     4      0
 *   Weapon      Rune Crossbow            90        0        0     0     0     0     0      0
 *   Body        Black D'hide Body        30        0       55    47    60    50    55      0
 *   Legs        Black D'hide Chaps       17        0       31    25    33    28    31      0
 *   Hands       Black D'hide Vambraces   11        0        6     5     7     8     0      0
 *   Feet        Snakeskin Boots           3        0        1     1     2     1     0      0
 *   Ammo        Adamant Bolts             0      100        0     0     0     0     0      0
 *                                      ----     ----     ----  ----  ----  ----  ----   ----
 *   TOTAL                               153      100       97    84   110    91    90      0
 */

/*
 * LOADOUT B: End-game — Masori (f) + Twisted Bow
 *
 *   Slot        Item                    Rng Atk  Rng Str  Stab  Slash Crush Magic Ranged Prayer
 *   ----        ----                    -------  -------  ----  ----- ----- ----- ------ ------
 *   Head        Masori mask (f)          12        2        8    10    12    12     9      1
 *   Cape        Ava's assembler           8        2        0     0     0     0     0      0
 *   Neck        Necklace of anguish      15        5        0     0     0     0     0      2
 *   Weapon      Twisted bow              70       20        0     0     0     0     0      0
 *   Body        Masori body (f)          43        4       59    52    64    74    60      1
 *   Legs        Masori chaps (f)         27        2       35    30    39    46    37      1
 *   Hands       Zaryte vambraces         18        2        8     8     8     5     8      1
 *   Feet        Pegasian boots           12        0        5     5     5     5     5      0
 *   Ring        Venator ring             10        2        0     0     0     0     0      0
 *   Ammo        Dragon arrows             0       60        0     0     0     0     0      0
 *   Shield      (none)                    0        0        0     0     0     0     0      0
 *                                      ----     ----     ----  ----  ----  ----  ----   ----
 *   TOTAL                               215       99      116   106   129   150   121      6
 */

#define FC_NUM_LOADOUTS FC_LOADOUT_COUNT

static const FcLoadout FC_LOADOUTS[FC_NUM_LOADOUTS] = {
    /* [FC_LOADOUT_BLACK_DHIDE_RCB] Mid-level — Black D'hide + Rune Crossbow */
    {
        .name         = "A: Black D'hide + RCB",
        .weapon_name  = "Rune crossbow",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_BLACK_DHIDE_RCB,
        .combat_style_profile = 9,
        .max_hp       = 700,   /* 70 HP */
        .max_prayer   = 430,   /* 43 prayer */
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 70,
        .ranged_lvl   = 70,
        .prayer_lvl   = 43,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_GENERIC_RANGED,
        .weapon_uses_ammo = 1,
        .weapon_speed = 5,
        .weapon_range = 7,
        .ranged_atk   = 153,   /* 2+90+30+17+11+3 */
        .ranged_str   = 100,   /* adamant bolts */
        .def_stab     = 97,    /* 4+0+55+31+6+1 */
        .def_slash    = 84,    /* 6+0+47+25+5+1 */
        .def_crush    = 110,   /* 8+0+60+33+7+2 */
        .def_magic    = 91,    /* 4+0+50+28+8+1 */
        .def_ranged   = 90,    /* 4+0+55+31+0+0 */
        .prayer_bonus = 0,
        .ammo         = 50000,
        .equipment_count = 7,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   1169, 0, "Coif"},
            {FC_EQUIP_SLOT_WEAPON, 9185, 0, "Rune crossbow"},
            {FC_EQUIP_SLOT_BODY,   2503, 0, "Black d'hide body"},
            {FC_EQUIP_SLOT_AMMO,   9143, 0, "Adamant bolts"},
            {FC_EQUIP_SLOT_LEGS,   2497, 0, "Black d'hide chaps"},
            {FC_EQUIP_SLOT_HANDS,  2491, 0, "Black d'hide vambraces"},
            {FC_EQUIP_SLOT_FEET,   6328, 0, "Snakeskin boots"},
        },
        .model_item_count = 6,
        .model_item_ids = {1169, 9185, 2503, 2497, 2491, 6328},
    },
    /* [FC_LOADOUT_SOTA_TBOW] End-game — Masori (f) + Twisted Bow */
    {
        .name         = "B: Masori (f) + TBow",
        .weapon_name  = "Twisted bow",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_SOTA_TBOW,
        .combat_style_profile = 25,
        .max_hp       = 990,   /* 99 HP */
        .max_prayer   = 990,   /* 99 prayer */
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 99,
        .ranged_lvl   = 99,
        .prayer_lvl   = 99,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_TWISTED_BOW,
        .weapon_uses_ammo = 1,
        .weapon_speed = 5,     /* rapid */
        .weapon_range = 10,
        .ranged_atk   = 215,   /* 12+8+15+70+43+27+18+12+10 */
        .ranged_str   = 99,    /* 2+2+5+20+4+2+2+0+2+60(dragon arrows) */
        .def_stab     = 116,   /* 8+1+0+0+59+35+8+5+0+0 */
        .def_slash    = 106,   /* 10+1+0+0+52+30+8+5+0+0 */
        .def_crush    = 129,   /* 12+1+0+0+64+39+8+5+0+0 */
        .def_magic    = 150,   /* 12+8+0+0+74+46+5+5+0+0 */
        .def_ranged   = 121,   /* 9+2+0+0+60+37+8+5+0+0 */
        .prayer_bonus = 6,     /* 1+0+2+0+1+1+1+0+0+0 */
        .ammo         = 50000,
        .equipment_count = 10,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   27235, 0, "Masori mask (f)"},
            {FC_EQUIP_SLOT_CAPE,   22109, 0, "Ava's assembler"},
            {FC_EQUIP_SLOT_NECK,   19547, 0, "Necklace of anguish"},
            {FC_EQUIP_SLOT_WEAPON, 20997, 0, "Twisted bow"},
            {FC_EQUIP_SLOT_BODY,   27238, 0, "Masori body (f)"},
            {FC_EQUIP_SLOT_AMMO,   11212, 0, "Dragon arrows"},
            {FC_EQUIP_SLOT_LEGS,   27241, 0, "Masori chaps (f)"},
            {FC_EQUIP_SLOT_HANDS,  26235, 0, "Zaryte vambraces"},
            {FC_EQUIP_SLOT_FEET,   13237, 0, "Pegasian boots"},
            {FC_EQUIP_SLOT_RING,   25487, 0, "Venator ring"},
        },
        .model_item_count = 8,
        .model_item_ids = {27235, 22109, 19547, 20997, 27238, 27241, 26235, 13237},
    },
    /* [FC_LOADOUT_LOW_DEF_RCB] Low-defence — Robin Hood + Red D'hide + RCB */
    {
        .name         = "C: 1 Def Robin + RCB",
        .weapon_name  = "Rune crossbow",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_LOW_DEF_RCB,
        .combat_style_profile = 9,
        .max_hp       = 550,   /* 55 HP */
        .max_prayer   = 430,   /* 43 prayer */
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 1,
        .ranged_lvl   = 61,
        .prayer_lvl   = 43,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_GENERIC_RANGED,
        .weapon_uses_ammo = 1,
        .weapon_speed = 5,     /* rapid rune crossbow */
        .weapon_range = 7,
        .ranged_atk   = 166,   /* 8+4+10+90+15+14+7+8+10 */
        .ranged_str   = 100,   /* adamant bolts */
        .def_stab     = 48,    /* 4+0+3+0+6+28+5+2+0 */
        .def_slash    = 49,    /* 6+1+3+0+9+22+5+3+0 */
        .def_crush    = 62,    /* 8+0+3+0+12+30+5+4+0 */
        .def_magic    = 42,    /* 4+4+3+0+6+20+3+2+0 */
        .def_ranged   = 46,    /* 4+0+3+0+6+28+5+0+0 */
        .prayer_bonus = 8,     /* glory + book of law */
        .ammo         = 50000,
        .equipment_count = 10,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   2581,  0, "Robin hood hat"},
            {FC_EQUIP_SLOT_CAPE,   10499, 0, "Ava's accumulator"},
            {FC_EQUIP_SLOT_NECK,   1704,  0, "Amulet of glory"},
            {FC_EQUIP_SLOT_WEAPON, 9185,  0, "Rune crossbow"},
            {FC_EQUIP_SLOT_BODY,   12596, 0, "Rangers' tunic"},
            {FC_EQUIP_SLOT_SHIELD, 12610, 0, "Book of law"},
            {FC_EQUIP_SLOT_AMMO,   9143,  0, "Adamant bolts"},
            {FC_EQUIP_SLOT_LEGS,   2495,  0, "Red d'hide chaps"},
            {FC_EQUIP_SLOT_HANDS,  11126, 0, "Combat bracelet"},
            {FC_EQUIP_SLOT_FEET,   2577,  0, "Ranger boots"},
        },
        .model_item_count = 9,
        .model_item_ids = {2581, 10499, 1704, 9185, 12596, 12610, 2495, 11126, 2577},
    },
    /* [FC_LOADOUT_RCB_PURE] Low-level 1-def Fight Caves rune crossbow pure */
    {
        .name         = "RCB Pure",
        .weapon_name  = "Rune crossbow",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_RCB_PURE,
        .combat_style_profile = 9,
        .max_hp       = 550,
        .max_prayer   = 430,
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 1,
        .ranged_lvl   = 61,
        .prayer_lvl   = 43,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_GENERIC_RANGED,
        .weapon_uses_ammo = 1,
        .weapon_speed = 5,
        .weapon_range = 7,
        .ranged_atk   = 166,
        .ranged_str   = 100,
        .def_stab     = 48,
        .def_slash    = 49,
        .def_crush    = 62,
        .def_magic    = 42,
        .def_ranged   = 46,
        .prayer_bonus = 8,
        .ammo         = 50000,
        .equipment_count = 10,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   2581,  0, "Robin hood hat"},
            {FC_EQUIP_SLOT_CAPE,   10499, 0, "Ava's accumulator"},
            {FC_EQUIP_SLOT_NECK,   1704,  0, "Amulet of glory"},
            {FC_EQUIP_SLOT_WEAPON, 9185,  0, "Rune crossbow"},
            {FC_EQUIP_SLOT_BODY,   12596, 0, "Rangers' tunic"},
            {FC_EQUIP_SLOT_SHIELD, 12610, 0, "Book of law"},
            {FC_EQUIP_SLOT_AMMO,   9143,  0, "Adamant bolts"},
            {FC_EQUIP_SLOT_LEGS,   2495,  0, "Red d'hide chaps"},
            {FC_EQUIP_SLOT_HANDS,  11126, 0, "Combat bracelet"},
            {FC_EQUIP_SLOT_FEET,   2577,  0, "Ranger boots"},
        },
        .model_item_count = 9,
        .model_item_ids = {2581, 10499, 1704, 9185, 12596, 12610, 2495, 11126, 2577},
    },
    /* [FC_LOADOUT_MSBI_PURE] Faster but weaker 1-def magic shortbow pure */
    {
        .name         = "MSB(i) Pure",
        .weapon_name  = "Magic shortbow (i)",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_MSBI_PURE,
        .combat_style_profile = 25,
        .max_hp       = 600,
        .max_prayer   = 430,
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 1,
        .ranged_lvl   = 70,
        .prayer_lvl   = 43,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_GENERIC_RANGED,
        .weapon_uses_ammo = 1,
        .weapon_speed = 3,
        .weapon_range = 7,
        .ranged_atk   = 141,
        .ranged_str   = 49,
        .def_stab     = 48,
        .def_slash    = 49,
        .def_crush    = 62,
        .def_magic    = 42,
        .def_ranged   = 46,
        .prayer_bonus = 3,
        .ammo         = 50000,
        .equipment_count = 9,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   2581,  0, "Robin hood hat"},
            {FC_EQUIP_SLOT_CAPE,   10499, 0, "Ava's accumulator"},
            {FC_EQUIP_SLOT_NECK,   1704,  0, "Amulet of glory"},
            {FC_EQUIP_SLOT_WEAPON, 12788, 0, "Magic shortbow (i)"},
            {FC_EQUIP_SLOT_AMMO,   892,   0, "Rune arrow"},
            {FC_EQUIP_SLOT_BODY,   12596, 0, "Rangers' tunic"},
            {FC_EQUIP_SLOT_LEGS,   2495,  0, "Red d'hide chaps"},
            {FC_EQUIP_SLOT_HANDS,  11126, 0, "Combat bracelet"},
            {FC_EQUIP_SLOT_FEET,   2577,  0, "Ranger boots"},
        },
        .model_item_count = 8,
        .model_item_ids = {2581, 10499, 1704, 12788, 12596, 2495, 11126, 2577},
    },
    /* [FC_LOADOUT_BLOWPIPE_PURE] Fast 1-def toxic blowpipe pure with loaded adamant darts */
    {
        .name         = "Blowpipe Pure",
        .weapon_name  = "Toxic blowpipe",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_BLOWPIPE_PURE,
        .combat_style_profile = 23,
        .max_hp       = 750,
        .max_prayer   = 430,
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 1,
        .ranged_lvl   = 75,
        .prayer_lvl   = 43,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_GENERIC_RANGED,
        .weapon_uses_ammo = 1,
        .weapon_speed = 2,
        .weapon_range = 5,
        .ranged_atk   = 101,
        .ranged_str   = 42,
        .def_stab     = 45,
        .def_slash    = 46,
        .def_crush    = 59,
        .def_magic    = 39,
        .def_ranged   = 43,
        .prayer_bonus = 2,
        .ammo         = 50000,
        .equipment_count = 9,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   2581,  0, "Robin hood hat"},
            {FC_EQUIP_SLOT_CAPE,   10499, 0, "Ava's accumulator"},
            {FC_EQUIP_SLOT_NECK,   19547, 0, "Necklace of anguish"},
            {FC_EQUIP_SLOT_WEAPON, 12926, 0, "Toxic blowpipe"},
            {FC_EQUIP_SLOT_AMMO,   810,   0, "Adamant dart"},
            {FC_EQUIP_SLOT_BODY,   12596, 0, "Rangers' tunic"},
            {FC_EQUIP_SLOT_LEGS,   2495,  0, "Red d'hide chaps"},
            {FC_EQUIP_SLOT_HANDS,  11126, 0, "Combat bracelet"},
            {FC_EQUIP_SLOT_FEET,   2577,  0, "Ranger boots"},
        },
        .model_item_count = 8,
        .model_item_ids = {2581, 10499, 19547, 12926, 12596, 2495, 11126, 2577},
    },
    /* [FC_LOADOUT_ACB_ARMADYL] Tankier high-level Armadyl crossbow + Armadyl armour */
    {
        .name         = "ACB Armadyl",
        .weapon_name  = "Armadyl crossbow",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_ACB_ARMADYL,
        .combat_style_profile = 9,
        .max_hp       = 800,
        .max_prayer   = 700,
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 75,
        .ranged_lvl   = 80,
        .prayer_lvl   = 70,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_GENERIC_RANGED,
        .weapon_uses_ammo = 1,
        .weapon_speed = 5,
        .weapon_range = 8,
        .ranged_atk   = 220,
        .ranged_str   = 129,
        .def_stab     = 112,
        .def_slash    = 100,
        .def_crush    = 123,
        .def_magic    = 139,
        .def_ranged   = 117,
        .prayer_bonus = 11,
        .ammo         = 50000,
        .equipment_count = 10,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   11826, 0, "Armadyl helmet"},
            {FC_EQUIP_SLOT_CAPE,   22109, 0, "Ava's assembler"},
            {FC_EQUIP_SLOT_NECK,   19547, 0, "Necklace of anguish"},
            {FC_EQUIP_SLOT_WEAPON, 11785, 0, "Armadyl crossbow"},
            {FC_EQUIP_SLOT_BODY,   11828, 0, "Armadyl chestplate"},
            {FC_EQUIP_SLOT_SHIELD, 12610, 0, "Book of law"},
            {FC_EQUIP_SLOT_AMMO,   21946, 0, "Diamond dragon bolts (e)"},
            {FC_EQUIP_SLOT_LEGS,   11830, 0, "Armadyl chainskirt"},
            {FC_EQUIP_SLOT_HANDS,  7462,  0, "Barrows gloves"},
            {FC_EQUIP_SLOT_FEET,   13237, 0, "Pegasian boots"},
        },
        .model_item_count = 9,
        .model_item_ids = {11826, 22109, 19547, 11785, 11828, 12610, 11830, 7462, 13237},
    },
    /* [FC_LOADOUT_BOWFA_CRYSTAL] Bowfa + crystal armour. */
    {
        .name         = "Bowfa Crystal",
        .weapon_name  = "Bow of faerdhinen (c)",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_BOWFA_CRYSTAL,
        .combat_style_profile = 25,
        .max_hp       = 850,
        .max_prayer   = 700,
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 75,
        .ranged_lvl   = 85,
        .prayer_lvl   = 70,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_BOW_OF_FAERDHINEN,
        .weapon_uses_ammo = 0,
        .crystal_piece_mask = FC_CRYSTAL_PIECE_ALL,
        .weapon_speed = 4,
        .weapon_range = 10,
        .ranged_atk   = 233,
        .ranged_str   = 113,
        .def_stab     = 102,
        .def_slash    = 85,
        .def_crush    = 110,
        .def_magic    = 107,
        .def_ranged   = 143,
        .prayer_bonus = 9,
        .ammo         = 0,
        .equipment_count = 8,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   23971, 0, "Crystal helm"},
            {FC_EQUIP_SLOT_CAPE,   22109, 0, "Ava's assembler"},
            {FC_EQUIP_SLOT_NECK,   19547, 0, "Necklace of anguish"},
            {FC_EQUIP_SLOT_WEAPON, 25867, 0, "Bow of faerdhinen (c)"},
            {FC_EQUIP_SLOT_BODY,   23975, 0, "Crystal body"},
            {FC_EQUIP_SLOT_LEGS,   23979, 0, "Crystal legs"},
            {FC_EQUIP_SLOT_HANDS,  7462,  0, "Barrows gloves"},
            {FC_EQUIP_SLOT_FEET,   13237, 0, "Pegasian boots"},
        },
        .model_item_count = 8,
        .model_item_ids = {23971, 22109, 19547, 25867, 23975, 23979, 7462, 13237},
    },
    /* [FC_LOADOUT_TBOW_MASORI] Max-ish Twisted bow + fortified Masori loadout */
    {
        .name         = "Tbow Masori",
        .weapon_name  = "Twisted bow",
        .player_model_id = FC_PLAYER_MODEL_BASE + FC_LOADOUT_TBOW_MASORI,
        .combat_style_profile = 25,
        .max_hp       = 990,
        .max_prayer   = 770,
        .attack_lvl   = 1,
        .strength_lvl = 1,
        .defence_lvl  = 80,
        .ranged_lvl   = 99,
        .prayer_lvl   = 77,
        .magic_lvl    = 1,
        .weapon_kind  = FC_WEAPON_TWISTED_BOW,
        .weapon_uses_ammo = 1,
        .weapon_speed = 5,
        .weapon_range = 10,
        .ranged_atk   = 205,
        .ranged_str   = 97,
        .def_stab     = 116,
        .def_slash    = 106,
        .def_crush    = 129,
        .def_magic    = 150,
        .def_ranged   = 121,
        .prayer_bonus = 6,
        .ammo         = 50000,
        .equipment_count = 9,
        .equipment = {
            {FC_EQUIP_SLOT_HEAD,   27235, 0, "Masori mask (f)"},
            {FC_EQUIP_SLOT_CAPE,   22109, 0, "Ava's assembler"},
            {FC_EQUIP_SLOT_NECK,   19547, 0, "Necklace of anguish"},
            {FC_EQUIP_SLOT_WEAPON, 20997, 0, "Twisted bow"},
            {FC_EQUIP_SLOT_BODY,   27238, 0, "Masori body (f)"},
            {FC_EQUIP_SLOT_AMMO,   11212, 0, "Dragon arrow"},
            {FC_EQUIP_SLOT_LEGS,   27241, 0, "Masori chaps (f)"},
            {FC_EQUIP_SLOT_HANDS,  26235, 0, "Zaryte vambraces"},
            {FC_EQUIP_SLOT_FEET,   13237, 0, "Pegasian boots"},
        },
        .model_item_count = 8,
        .model_item_ids = {27235, 22109, 19547, 20997, 27238, 27241, 26235, 13237},
    },
};

/* ======================================================================== */
/* Compile-time constants (used by fc_types.h and training backend)           */
/* These pull from the active loadout index set above.                       */
/* ======================================================================== */

#define FC_PLAYER_MAX_HP        (FC_LOADOUTS[FC_ACTIVE_LOADOUT].max_hp)
#define FC_PLAYER_MAX_PRAYER    (FC_LOADOUTS[FC_ACTIVE_LOADOUT].max_prayer)
#define FC_PLAYER_ATTACK_LVL    (FC_LOADOUTS[FC_ACTIVE_LOADOUT].attack_lvl)
#define FC_PLAYER_STRENGTH_LVL  (FC_LOADOUTS[FC_ACTIVE_LOADOUT].strength_lvl)
#define FC_PLAYER_DEFENCE_LVL   (FC_LOADOUTS[FC_ACTIVE_LOADOUT].defence_lvl)
#define FC_PLAYER_RANGED_LVL    (FC_LOADOUTS[FC_ACTIVE_LOADOUT].ranged_lvl)
#define FC_PLAYER_PRAYER_LVL    (FC_LOADOUTS[FC_ACTIVE_LOADOUT].prayer_lvl)
#define FC_PLAYER_MAGIC_LVL     (FC_LOADOUTS[FC_ACTIVE_LOADOUT].magic_lvl)
#define FC_PLAYER_WEAPON_KIND   (FC_LOADOUTS[FC_ACTIVE_LOADOUT].weapon_kind)
#define FC_PLAYER_WEAPON_USES_AMMO (FC_LOADOUTS[FC_ACTIVE_LOADOUT].weapon_uses_ammo)
#define FC_PLAYER_CRYSTAL_PIECE_MASK (FC_LOADOUTS[FC_ACTIVE_LOADOUT].crystal_piece_mask)
#define FC_PLAYER_WEAPON_SPEED  (FC_LOADOUTS[FC_ACTIVE_LOADOUT].weapon_speed)
#define FC_PLAYER_WEAPON_RANGE  (FC_LOADOUTS[FC_ACTIVE_LOADOUT].weapon_range)
#define FC_EQUIP_RANGED_ATK     (FC_LOADOUTS[FC_ACTIVE_LOADOUT].ranged_atk)
#define FC_EQUIP_RANGED_STR     (FC_LOADOUTS[FC_ACTIVE_LOADOUT].ranged_str)
#define FC_EQUIP_DEF_STAB       (FC_LOADOUTS[FC_ACTIVE_LOADOUT].def_stab)
#define FC_EQUIP_DEF_SLASH      (FC_LOADOUTS[FC_ACTIVE_LOADOUT].def_slash)
#define FC_EQUIP_DEF_CRUSH      (FC_LOADOUTS[FC_ACTIVE_LOADOUT].def_crush)
#define FC_EQUIP_DEF_MAGIC      (FC_LOADOUTS[FC_ACTIVE_LOADOUT].def_magic)
#define FC_EQUIP_DEF_RANGED     (FC_LOADOUTS[FC_ACTIVE_LOADOUT].def_ranged)
#define FC_EQUIP_PRAYER_BONUS   (FC_LOADOUTS[FC_ACTIVE_LOADOUT].prayer_bonus)
#define FC_PLAYER_AMMO          (FC_LOADOUTS[FC_ACTIVE_LOADOUT].ammo)

#endif /* FC_PLAYER_INIT_H */
