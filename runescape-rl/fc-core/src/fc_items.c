#include "fc_items_internal.h"
#include "fc_api.h"
#include "fc_magic.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pinned to the existing FcLoadout balance, including legacy d'hide defence
 * and Pegasian strength. Equipment switching must not rebalance training.
 * Additional magic/melee items and runes are opt-in, not starting gear. */
enum { AMMO_NONE, AMMO_ARROW, AMMO_BOLT, AMMO_LOADED_DART };
static const FcItemDef ITEMS[] = {
    /* Tiered presets: pinned OSRS Wiki equipment bonuses (September 2026). */
    {.id=10828, .name="Helm of neitiznot", .slot=FC_EQUIP_SLOT_HEAD,
     .prayer=3, .melee_strength=3, .defence_level=55, .defence={31,29,34,3,30}},
    {.id=6585, .name="Amulet of fury", .slot=FC_EQUIP_SLOT_NECK,
     .ranged_attack=10, .prayer=5, .melee_attack=10, .melee_strength=8, .magic_attack=10, .defence={15,15,15,15,15}},
    {.id=10551, .name="Fighter torso", .slot=FC_EQUIP_SLOT_BODY,
     .melee_strength=4, .magic_attack=-40, .defence_level=40, .defence={62,85,62,-10,67}},
    {.id=4087, .name="Dragon platelegs", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=-11, .magic_attack=-21, .defence_level=60, .defence={68,66,63,-4,65}},
    {.id=4587, .name="Dragon scimitar", .slot=FC_EQUIP_SLOT_WEAPON,
     .melee_attack=-2, .melee_strength=66, .melee_stab=10, .melee_slash=69, .attack_level=60,
     .weapon_kind=FC_WEAPON_MELEE, .speed=4, .range=1, .visual_profile=-4, .melee_type=2, .defence={0,1,0,0,0}},
    {.id=12954, .name="Dragon defender", .slot=FC_EQUIP_SLOT_SHIELD,
     .ranged_attack=-2, .melee_attack=23, .melee_strength=6, .melee_stab=2, .melee_slash=1, .magic_attack=-3,
     .defence_level=60, .defence={25,24,23,-3,-2}},
    {.id=11840, .name="Dragon boots", .slot=FC_EQUIP_SLOT_FEET,
     .ranged_attack=-1, .melee_strength=4, .magic_attack=-3, .defence_level=60, .defence={16,17,18,0,0}},
    {.id=29028, .name="Blood moon helm", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=-2, .melee_strength=4, .magic_attack=-6, .strength_level=75, .defence_level=50, .defence={20,31,34,7,29}},
    {.id=29022, .name="Blood moon chestplate", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=-10, .melee_strength=4, .magic_attack=-15, .strength_level=75, .defence_level=50, .defence={60,80,80,40,79}},
    {.id=29025, .name="Blood moon tassets", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=-7, .melee_strength=2, .magic_attack=-21, .strength_level=75, .defence_level=50, .defence={30,50,49,32,46}},
    {.id=28997, .name="Dual macuahuitl", .slot=FC_EQUIP_SLOT_WEAPON,
     .melee_attack=121, .melee_strength=81, .melee_stab=-6, .melee_slash=-125, .magic_attack=-4, .attack_level=70,
     .strength_level=75, .two_handed=1, .weapon_kind=FC_WEAPON_MELEE, .speed=4, .range=1, .visual_profile=-6,
     .defence={0,0,0,0,0}},
    {.id=11773, .name="Berserker ring (i)", .slot=FC_EQUIP_SLOT_RING,
     .melee_strength=8, .defence={0,0,8,0,0}},
    {.id=24271, .name="Neitiznot faceguard", .slot=FC_EQUIP_SLOT_HEAD,
     .prayer=3, .melee_strength=6, .defence_level=70, .defence={36,34,38,3,34}},
    {.id=19553, .name="Amulet of torture", .slot=FC_EQUIP_SLOT_NECK,
     .prayer=2, .melee_attack=15, .melee_strength=10, .hitpoints_level=75, .defence={0,0,0,0,0}},
    {.id=11832, .name="Bandos chestplate", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=-10, .prayer=1, .melee_strength=4, .magic_attack=-15, .defence_level=65, .defence={98,93,105,-6,133}},
    {.id=11834, .name="Bandos tassets", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=-7, .prayer=1, .melee_strength=2, .magic_attack=-21, .defence_level=65, .defence={71,63,66,-4,93}},
    {.id=26219, .name="Osmumten's fang", .slot=FC_EQUIP_SLOT_WEAPON,
     .melee_strength=103, .melee_stab=105, .melee_slash=75, .attack_level=82, .weapon_kind=FC_WEAPON_MELEE, .speed=5,
     .range=1, .visual_profile=-5, .melee_type=1, .defence={0,0,0,0,0}},
    {.id=22322, .name="Avernic defender", .slot=FC_EQUIP_SLOT_SHIELD,
     .ranged_attack=-4, .melee_attack=28, .melee_strength=8, .melee_stab=2, .melee_slash=1, .magic_attack=-5,
     .attack_level=70, .defence_level=70, .defence={30,29,28,-5,-4}},
    {.id=22981, .name="Ferocious gloves", .slot=FC_EQUIP_SLOT_HANDS,
     .ranged_attack=-16, .melee_attack=16, .melee_strength=14, .magic_attack=-16, .attack_level=80, .defence_level=80, .defence={0,0,0,0,0}},
    {.id=13239, .name="Primordial boots", .slot=FC_EQUIP_SLOT_FEET,
     .ranged_attack=-1, .melee_attack=2, .melee_strength=5, .magic_attack=-4, .strength_level=75, .defence_level=75, .defence={22,22,22,0,0}},
    {.id=22947, .name="Rada's blessing 4", .slot=FC_EQUIP_SLOT_AMMO,
     .prayer=2, .defence={0,0,0,0,0}},
    {.id=26382, .name="Torva full helm", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=-5, .prayer=1, .melee_strength=8, .magic_attack=-5, .defence_level=80, .defence={59,60,62,-2,57}},
    {.id=21295, .name="Infernal cape", .slot=FC_EQUIP_SLOT_CAPE,
     .ranged_attack=1, .prayer=2, .melee_attack=4, .melee_strength=8, .magic_attack=1, .defence={12,12,12,12,12}},
    {.id=29801, .name="Amulet of rancour", .slot=FC_EQUIP_SLOT_NECK,
     .ranged_attack=-8, .prayer=2, .melee_attack=25, .melee_strength=12, .magic_attack=-6, .hitpoints_level=90, .defence={0,0,0,0,0}},
    {.id=26384, .name="Torva platebody", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=-14, .prayer=1, .melee_strength=6, .magic_attack=-18, .defence_level=80, .defence={117,111,117,-11,142}},
    {.id=26386, .name="Torva platelegs", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=-11, .prayer=1, .melee_strength=4, .magic_attack=-24, .defence_level=80, .defence={87,78,79,-9,102}},
    {.id=22325, .name="Scythe of vitur", .slot=FC_EQUIP_SLOT_WEAPON,
     .melee_attack=30, .melee_strength=75, .melee_stab=40, .melee_slash=95, .magic_attack=-6, .attack_level=80,
     .strength_level=90, .two_handed=1, .weapon_kind=FC_WEAPON_MELEE, .speed=5, .range=1, .visual_profile=-7,
     .melee_type=2, .defence={-2,8,10,0,0}},
    {.id=31097, .name="Avernic treads (max)", .slot=FC_EQUIP_SLOT_FEET,
     .ranged_attack=15, .ranged_strength=3, .melee_attack=5, .melee_strength=6, .magic_attack=11,
     .magic_damage_permille=20, .defence_level=80, .defence={21,25,25,10,10}},
    {.id=28307, .name="Ultor ring", .slot=FC_EQUIP_SLOT_RING,
     .melee_strength=12, .hitpoints_level=90, .defence={0,0,0,0,0}},
    {.id=3749, .name="Archer helm", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=6, .melee_attack=-5, .magic_attack=-5, .defence_level=45, .defence={6,8,10,6,6}},
    {.id=2501, .name="Red d'hide body", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=25, .magic_attack=-15, .defence_level=40, .ranged_level=60, .defence={26,34,36,36,45}},
    {.id=11926, .name="Odium ward", .slot=FC_EQUIP_SLOT_SHIELD,
     .ranged_attack=12, .ranged_strength=4, .melee_attack=-12, .magic_attack=-8, .defence_level=60, .defence={0,0,0,24,52}},
    {.id=6733, .name="Archers ring", .slot=FC_EQUIP_SLOT_RING,
     .ranged_attack=4, .defence={0,0,0,0,4}},
    {.id=9243, .name="Diamond bolts (e)", .slot=FC_EQUIP_SLOT_AMMO,
     .ranged_strength=105, .stackable=1, .ammo_kind=AMMO_BOLT, .defence={0,0,0,0,0}},
    {.id=12512, .name="Armadyl coif", .slot=FC_EQUIP_SLOT_HEAD,
     .ranged_attack=7, .prayer=1, .magic_attack=-1, .defence_level=40, .ranged_level=70, .defence={4,7,10,4,8}},
    {.id=12508, .name="Armadyl d'hide body", .slot=FC_EQUIP_SLOT_BODY,
     .ranged_attack=30, .prayer=1, .magic_attack=-15, .defence_level=40, .ranged_level=70, .defence={55,47,60,50,55}},
    {.id=12510, .name="Armadyl chaps", .slot=FC_EQUIP_SLOT_LEGS,
     .ranged_attack=17, .prayer=1, .magic_attack=-10, .defence_level=40, .ranged_level=70, .defence={31,25,33,28,31}},
    {.id=19930, .name="Armadyl d'hide boots", .slot=FC_EQUIP_SLOT_FEET,
     .ranged_attack=7, .prayer=1, .magic_attack=-10, .defence_level=40, .ranged_level=70, .defence={4,4,4,4,4}},
    {.id=11771, .name="Archers ring (i)", .slot=FC_EQUIP_SLOT_RING,
     .ranged_attack=8, .defence={0,0,0,0,8}},
    {.id=21944, .name="Ruby dragon bolts (e)", .slot=FC_EQUIP_SLOT_AMMO,
     .ranged_strength=122, .stackable=1, .ammo_kind=AMMO_BOLT, .ammo_tier=1, .defence={0,0,0,0,0}},
    {.id=4089, .name="Mystic hat", .slot=FC_EQUIP_SLOT_HEAD,
     .magic_attack=4, .magic_level=40, .defence_level=20, .defence={0,0,0,4,0}},
    {.id=4091, .name="Mystic robe top", .slot=FC_EQUIP_SLOT_BODY,
     .magic_attack=20, .magic_level=40, .defence_level=20, .defence={0,0,0,20,0}},
    {.id=4093, .name="Mystic robe bottom", .slot=FC_EQUIP_SLOT_LEGS,
     .magic_attack=15, .magic_level=40, .defence_level=20, .defence={0,0,0,15,0}},
    {.id=4095, .name="Mystic gloves", .slot=FC_EQUIP_SLOT_HANDS,
     .magic_attack=3, .magic_level=40, .defence_level=20, .defence={0,0,0,3,0}},
    {.id=4097, .name="Mystic boots", .slot=FC_EQUIP_SLOT_FEET,
     .magic_attack=3, .magic_level=40, .defence_level=20, .defence={0,0,0,3,0}},
    {.id=1727, .name="Amulet of magic", .slot=FC_EQUIP_SLOT_NECK,
     .magic_attack=10, .defence={0,0,0,0,0}},
    {.id=6731, .name="Seers ring", .slot=FC_EQUIP_SLOT_RING,
     .magic_attack=6, .magic_damage_permille=2, .defence={0,0,0,6,0}},
    {.id=29019, .name="Blue moon helm", .slot=FC_EQUIP_SLOT_HEAD,
     .melee_strength=3, .magic_attack=6, .magic_damage_permille=10, .magic_level=75, .defence_level=50, .defence={0,0,10,6,0}},
    {.id=29013, .name="Blue moon chestplate", .slot=FC_EQUIP_SLOT_BODY,
     .melee_strength=2, .magic_attack=30, .magic_damage_permille=10, .magic_level=75, .defence_level=50, .defence={0,0,51,28,0}},
    {.id=29016, .name="Blue moon tassets", .slot=FC_EQUIP_SLOT_LEGS,
     .melee_strength=1, .magic_attack=22, .magic_damage_permille=10, .magic_level=75, .defence_level=50, .defence={0,0,23,32,0}},
    {.id=30634, .name="Twinflame staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .melee_attack=28, .melee_strength=22, .melee_stab=-21, .melee_slash=-29, .magic_attack=12, .magic_level=60,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=6, .range=1, .visual_profile=-2, .autocast=1, .infinite_runes=10,
     .defence={2,3,1,12,0}},
    {.id=6920, .name="Infinity boots", .slot=FC_EQUIP_SLOT_FEET,
     .magic_attack=5, .magic_level=50, .defence_level=25, .defence={0,0,0,5,0}},
    {.id=11770, .name="Seers ring (i)", .slot=FC_EQUIP_SLOT_RING,
     .magic_attack=12, .magic_damage_permille=5, .defence={0,0,0,12,0}},
    {.id=25985, .name="Elidinis' ward", .slot=FC_EQUIP_SLOT_SHIELD,
     .prayer=1, .magic_attack=5, .magic_damage_permille=30, .magic_level=80, .defence_level=80, .prayer_level=80, .defence={5,3,9,0,6}},
    {.id=27251, .name="Elidinis' ward (f)", .slot=FC_EQUIP_SLOT_SHIELD,
     .prayer=4, .magic_attack=25, .magic_damage_permille=50, .magic_level=80, .defence_level=80, .prayer_level=80, .defence={53,55,73,2,52}},
    {.id=31106, .name="Confliction gauntlets", .slot=FC_EQUIP_SLOT_HANDS,
     .ranged_attack=-4, .prayer=2, .magic_attack=20, .magic_damage_permille=70, .hitpoints_level=90, .defence={15,18,7,5,5}},
    {.id=6889, .name="Mage's book", .slot=FC_EQUIP_SLOT_SHIELD,
     .magic_attack=15, .magic_damage_permille=20, .magic_level=60, .defence={0,0,0,15,0}},
    {.id=2412, .name="Saradomin cape", .slot=FC_EQUIP_SLOT_CAPE,
     .magic_attack=10, .magic_level=60, .defence={1,1,2,10,0}},
    {.id=563, .name="Law rune", .slot=-1, .stackable=1},
    {.id=564, .name="Cosmic rune", .slot=-1, .stackable=1},
    {.id=9075, .name="Astral rune", .slot=-1, .stackable=1},
    /* Minimal melee test kit. Bonuses: OSRS DPS equipment data. Only the
     * mace's accurate/crush style is exposed by the current simulator. */
    {.id=1432, .name="Rune mace", .slot=FC_EQUIP_SLOT_WEAPON, .attack_level=40,
     .melee_attack=39, .melee_strength=36, .prayer=4,
     .weapon_kind=FC_WEAPON_MELEE, .speed=4, .range=1, .visual_profile=-3},
    {.id=1163, .name="Rune full helm", .slot=FC_EQUIP_SLOT_HEAD, .defence_level=40,
     .ranged_attack=-3, .magic_attack=-6, .defence={30,32,27,-1,30}},
    {.id=1127, .name="Rune platebody", .slot=FC_EQUIP_SLOT_BODY, .defence_level=40,
     .ranged_attack=-15, .magic_attack=-30, .defence={82,80,72,-6,80}},
    {.id=1079, .name="Rune platelegs", .slot=FC_EQUIP_SLOT_LEGS, .defence_level=40,
     .ranged_attack=-11, .magic_attack=-21, .defence={51,49,47,-4,49}},
    {.id=1201, .name="Rune kiteshield", .slot=FC_EQUIP_SLOT_SHIELD, .defence_level=40,
     .ranged_attack=-3, .magic_attack=-8, .defence={44,48,46,-1,46}},
    {.id=6570, .name="Fire cape", .slot=FC_EQUIP_SLOT_CAPE,
     .melee_attack=1, .melee_strength=4, .ranged_attack=1, .magic_attack=1,
     .defence={11,11,11,11,11}, .prayer=2},
    {.id=3105, .name="Climbing boots", .slot=FC_EQUIP_SLOT_FEET,
     .melee_strength=2, .defence={0,2,2,0,0}},
    {.id=6737, .name="Berserker ring", .slot=FC_EQUIP_SLOT_RING,
     .melee_strength=4, .defence={0,0,4,0,0}},
    {.id=1169, .name="Coif", .slot=FC_EQUIP_SLOT_HEAD, .magic_attack=-1,
     .ranged_attack=2, .ranged_strength=0, .defence={4,6,8,4,4}, .prayer=0,
     .ranged_level=20, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=22109, .name="Ava's assembler", .slot=FC_EQUIP_SLOT_CAPE,
     .ranged_attack=8, .ranged_strength=2, .defence={1,1,1,8,2}, .prayer=0,
     .ranged_level=70, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=19547, .name="Necklace of anguish", .slot=FC_EQUIP_SLOT_NECK,
     .ranged_attack=15, .ranged_strength=5, .defence={0,0,0,0,0}, .prayer=2,
     .ranged_level=0, .defence_level=0, .hitpoints_level=75, .melee_attack=0, .melee_strength=0},
    {.id=27235, .name="Masori mask (f)", .slot=FC_EQUIP_SLOT_HEAD, .magic_attack=-1,
     .ranged_attack=12, .ranged_strength=2, .defence={8,10,12,12,9}, .prayer=1,
     .ranged_level=80, .defence_level=80, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=27238, .name="Masori body (f)", .slot=FC_EQUIP_SLOT_BODY, .magic_attack=-4,
     .ranged_attack=43, .ranged_strength=4, .defence={59,52,64,74,60}, .prayer=1,
     .ranged_level=80, .defence_level=80, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=27241, .name="Masori chaps (f)", .slot=FC_EQUIP_SLOT_LEGS, .magic_attack=-2,
     .ranged_attack=27, .ranged_strength=2, .defence={35,30,39,46,37}, .prayer=1,
     .ranged_level=80, .defence_level=80, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=26235, .name="Zaryte vambraces", .slot=FC_EQUIP_SLOT_HANDS,
     .ranged_attack=18, .ranged_strength=2, .defence={8,8,8,5,8}, .prayer=1,
     .ranged_level=80, .defence_level=45, .hitpoints_level=0, .melee_attack=-8, .melee_strength=0},
    {.id=13237, .name="Pegasian boots", .slot=FC_EQUIP_SLOT_FEET, .magic_attack=-12,
     .ranged_attack=12, .ranged_strength=0, .defence={5,5,5,5,5}, .prayer=0,
     .ranged_level=75, .defence_level=75, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=28310, .name="Venator ring", .slot=FC_EQUIP_SLOT_RING,
     .ranged_attack=10, .ranged_strength=2, .defence={0,0,0,0,0}, .prayer=0,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2503, .name="Black d'hide body", .slot=FC_EQUIP_SLOT_BODY, .magic_attack=-15,
     .ranged_attack=30, .ranged_strength=0, .defence={55,47,60,50,55}, .prayer=0,
     .ranged_level=70, .defence_level=40, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2497, .name="Black d'hide chaps", .slot=FC_EQUIP_SLOT_LEGS, .magic_attack=-10,
     .ranged_attack=17, .ranged_strength=0, .defence={31,25,33,28,31}, .prayer=0,
     .ranged_level=70, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2491, .name="Black d'hide vambraces", .slot=FC_EQUIP_SLOT_HANDS, .magic_attack=-10,
     .ranged_attack=11, .ranged_strength=0, .defence={6,5,7,8,0}, .prayer=0,
     .ranged_level=70, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=6328, .name="Snakeskin boots", .slot=FC_EQUIP_SLOT_FEET, .magic_attack=-10,
     .ranged_attack=3, .ranged_strength=0, .defence={1,1,2,1,0}, .prayer=0,
     .ranged_level=30, .defence_level=30, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2581, .name="Robin hood hat", .slot=FC_EQUIP_SLOT_HEAD, .magic_attack=-10,
     .ranged_attack=8, .ranged_strength=0, .defence={4,6,8,4,4}, .prayer=0,
     .ranged_level=40, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=10499, .name="Ava's accumulator", .slot=FC_EQUIP_SLOT_CAPE,
     .ranged_attack=4, .ranged_strength=0, .defence={0,1,0,4,0}, .prayer=0,
     .ranged_level=50, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=1704, .name="Amulet of glory", .slot=FC_EQUIP_SLOT_NECK, .magic_attack=10,
     .ranged_attack=10, .ranged_strength=0, .defence={3,3,3,3,3}, .prayer=3,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=10, .melee_strength=6},
    {.id=12596, .name="Rangers' tunic", .slot=FC_EQUIP_SLOT_BODY, .magic_attack=-15,
     .ranged_attack=15, .ranged_strength=0, .defence={6,9,12,6,6}, .prayer=0,
     .ranged_level=40, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=12610, .name="Book of law", .slot=FC_EQUIP_SLOT_SHIELD,
     .ranged_attack=10, .ranged_strength=0, .defence={0,0,0,0,0}, .prayer=5,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=2495, .name="Red d'hide chaps", .slot=FC_EQUIP_SLOT_LEGS, .magic_attack=-10,
     .ranged_attack=14, .ranged_strength=0, .defence={28,22,30,20,28}, .prayer=0,
     .ranged_level=60, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=11126, .name="Combat bracelet", .slot=FC_EQUIP_SLOT_HANDS, .magic_attack=3,
     .ranged_attack=7, .ranged_strength=0, .defence={5,5,5,3,5}, .prayer=0,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=7, .melee_strength=6},
    {.id=2577, .name="Ranger boots", .slot=FC_EQUIP_SLOT_FEET, .magic_attack=-10,
     .ranged_attack=8, .ranged_strength=0, .defence={2,3,4,2,0}, .prayer=0,
     .ranged_level=40, .defence_level=0, .hitpoints_level=0, .melee_attack=0, .melee_strength=0},
    {.id=11826, .name="Armadyl helmet", .slot=FC_EQUIP_SLOT_HEAD, .magic_attack=-5,
     .ranged_attack=10, .ranged_strength=0, .defence={6,8,10,10,8}, .prayer=1,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=-5, .melee_strength=0},
    {.id=11828, .name="Armadyl chestplate", .slot=FC_EQUIP_SLOT_BODY, .magic_attack=-15,
     .ranged_attack=33, .ranged_strength=0, .defence={56,48,61,70,57}, .prayer=1,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=-7, .melee_strength=0},
    {.id=11830, .name="Armadyl chainskirt", .slot=FC_EQUIP_SLOT_LEGS, .magic_attack=-10,
     .ranged_attack=20, .ranged_strength=0, .defence={32,26,34,40,33}, .prayer=1,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=-6, .melee_strength=0},
    {.id=7462, .name="Barrows gloves", .slot=FC_EQUIP_SLOT_HANDS, .magic_attack=6,
     .ranged_attack=12, .ranged_strength=0, .defence={12,12,12,6,12}, .prayer=0,
     .ranged_level=0, .defence_level=0, .hitpoints_level=0, .melee_attack=12, .melee_strength=12},
    {.id=23971, .name="Crystal helm", .slot=FC_EQUIP_SLOT_HEAD, .magic_attack=-10,
     .ranged_attack=9, .ranged_strength=0, .defence={12,8,14,10,18}, .prayer=2,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=0, .melee_strength=0, .crystal_piece=FC_CRYSTAL_PIECE_HELM},
    {.id=23975, .name="Crystal body", .slot=FC_EQUIP_SLOT_BODY, .magic_attack=-18,
     .ranged_attack=31, .ranged_strength=0, .defence={46,38,48,44,68}, .prayer=3,
     .ranged_level=70, .defence_level=70, .hitpoints_level=0, .melee_attack=0, .melee_strength=0, .crystal_piece=FC_CRYSTAL_PIECE_BODY},
    {.id=23979, .name="Crystal legs", .slot=FC_EQUIP_SLOT_LEGS, .magic_attack=-12,
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

    /* Magic equipment: OSRS Wiki calculator snapshot plus RuneC B237 item requirements.
     * Per-mille bonuses retain fractional percentages and round once per cast. */
    {.id=27665, .name="Accursed sceptre", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=70, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=22, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=17000, .depleted_item_id=27662, .powered_divisor=3,
     .powered_multiplier=1, .powered_offset=-18},
    {.id=27662, .name="Accursed sceptre (u)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=70, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=22, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=17000, .depleted_item_id=27662, .powered_divisor=0,
     .powered_multiplier=1, .powered_offset=-18},
    {.id=21018, .name="Ancestral hat", .slot=FC_EQUIP_SLOT_HEAD,
     .magic_level=75, .attack_level=0, .defence_level=65, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=30, .prayer=0,
     .ranged_attack=-2,
     .defence={12,11,13,5,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=21024, .name="Ancestral robe bottom", .slot=FC_EQUIP_SLOT_LEGS,
     .magic_level=75, .attack_level=0, .defence_level=65, .hitpoints_level=0,
     .magic_attack=26, .magic_damage_permille=30, .prayer=0,
     .ranged_attack=-7,
     .defence={27,24,30,20,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=21021, .name="Ancestral robe top", .slot=FC_EQUIP_SLOT_BODY,
     .magic_level=75, .attack_level=0, .defence_level=65, .hitpoints_level=0,
     .magic_attack=35, .magic_damage_permille=30, .prayer=0,
     .ranged_attack=-8,
     .defence={42,31,51,28,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=4675, .name="Ancient staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=50, .attack_level=50, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=-1,
     .defence={2,3,1,15,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=3, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=1391, .name="Battlestaff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=0, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=12, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,12,0}, .melee_attack=25, .melee_strength=32,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=777, .name="Chaos gauntlets", .slot=FC_EQUIP_SLOT_HANDS,
     .magic_level=0, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=0, .magic_damage_permille=0, .prayer=0,
     .defence={8,9,7,0,0}, .melee_attack=2, .melee_strength=2,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=22516, .name="Dawnbringer", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=0, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-6},
    {.id=13235, .name="Eternal boots", .slot=FC_EQUIP_SLOT_FEET,
     .magic_level=75, .attack_level=0, .defence_level=75, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=10, .prayer=0,
     .defence={5,5,5,8,5}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=31113, .name="Eye of ayak", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=83, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=30, .magic_damage_permille=0, .prayer=2,
     .defence={1,5,5,10,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=3, .range=6, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=50000, .depleted_item_id=31115, .powered_divisor=3,
     .powered_multiplier=1, .powered_offset=-18},
    {.id=31115, .name="Eye of ayak (uncharged)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=83, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=30, .magic_damage_permille=0, .prayer=2,
     .defence={1,5,5,10,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=3, .range=6, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=50000, .depleted_item_id=31115, .powered_divisor=0,
     .powered_multiplier=1, .powered_offset=-18},
    {.id=2416, .name="Guthix staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=60, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=6, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,6,0}, .melee_attack=6, .melee_strength=2,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=24423, .name="Harmonised nightmare staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=82, .attack_level=0, .defence_level=0, .hitpoints_level=50,
     .magic_attack=16, .magic_damage_permille=150, .prayer=0,
     .defence={0,0,0,14,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=25731, .name="Holy sanguinesti staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=82, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .ranged_attack=-4,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=25733, .powered_divisor=3, .powered_multiplier=1, .powered_offset=0},
    {.id=25733, .name="Holy sanguinesti staff (uncharged)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=82, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .ranged_attack=-4,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=25733, .powered_divisor=0, .powered_multiplier=1, .powered_offset=0},
    {.id=1409, .name="Iban's staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=50, .attack_level=50, .defence_level=0, .hitpoints_level=0,
     .magic_attack=10, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,10,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=8, .infinite_runes=0, .charge_capacity=120, .depleted_item_id=0},
    {.id=12658, .name="Iban's staff (u)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=50, .attack_level=50, .defence_level=0, .hitpoints_level=0,
     .magic_attack=10, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,10,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=8, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=0},
    {.id=21791, .name="Imbued saradomin cape", .slot=FC_EQUIP_SLOT_CAPE,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=20, .prayer=0,
     .defence={3,3,3,15,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=21006, .name="Kodai wand", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=80, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=28, .magic_damage_permille=150, .prayer=0,
     .defence={0,3,3,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=3, .infinite_runes=2, .charge_capacity=0, .depleted_item_id=0},
    {.id=28313, .name="Magus ring", .slot=FC_EQUIP_SLOT_RING,
     .magic_level=0, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=20, .prayer=0,
     .defence={0,0,0,0,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=6914, .name="Master wand", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=60, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=20, .magic_damage_permille=100, .prayer=0,
     .defence={0,0,0,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=3, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=1405, .name="Mystic air staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=40, .attack_level=40, .defence_level=0, .hitpoints_level=0,
     .magic_attack=14, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,14,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=1, .charge_capacity=0, .depleted_item_id=0},
    {.id=1407, .name="Mystic earth staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=40, .attack_level=40, .defence_level=0, .hitpoints_level=0,
     .magic_attack=14, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,14,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=4, .charge_capacity=0, .depleted_item_id=0},
    {.id=1401, .name="Mystic fire staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=40, .attack_level=40, .defence_level=0, .hitpoints_level=0,
     .magic_attack=14, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,14,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=8, .charge_capacity=0, .depleted_item_id=0},
    {.id=12000, .name="Mystic smoke staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=40, .attack_level=40, .defence_level=0, .hitpoints_level=0,
     .magic_attack=14, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,14,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=9, .charge_capacity=0, .depleted_item_id=0},
    {.id=1403, .name="Mystic water staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=40, .attack_level=40, .defence_level=0, .hitpoints_level=0,
     .magic_attack=14, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,14,0}, .melee_attack=40, .melee_strength=50,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=2, .charge_capacity=0, .depleted_item_id=0},
    {.id=24422, .name="Nightmare staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=72, .attack_level=0, .defence_level=0, .hitpoints_level=50,
     .magic_attack=16, .magic_damage_permille=150, .prayer=0,
     .defence={0,0,0,14,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=3, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=12002, .name="Occult necklace", .slot=FC_EQUIP_SLOT_NECK,
     .magic_level=70, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=12, .magic_damage_permille=50, .prayer=2,
     .defence={0,0,0,0,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=22323, .name="Sanguinesti staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .ranged_attack=-4,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=22481, .powered_divisor=3, .powered_multiplier=1, .powered_offset=0},
    {.id=22481, .name="Sanguinesti staff (uncharged)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .ranged_attack=-4,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=22481, .powered_divisor=0, .powered_multiplier=1, .powered_offset=0},
    {.id=2415, .name="Saradomin staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=60, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=6, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,6,0}, .melee_attack=6, .melee_strength=2,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=4170, .name="Slayer's staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=12, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,10,0}, .melee_attack=25, .melee_strength=35,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=277, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=11998, .name="Smoke battlestaff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=30, .attack_level=30, .defence_level=0, .hitpoints_level=0,
     .magic_attack=12, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,12,0}, .melee_attack=28, .melee_strength=35,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=9, .charge_capacity=0, .depleted_item_id=0},
    {.id=1381, .name="Staff of air", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=1, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=10, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,10,0}, .melee_attack=7, .melee_strength=3,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=1, .charge_capacity=0, .depleted_item_id=0},
    {.id=1385, .name="Staff of earth", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=1, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=10, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,10,0}, .melee_attack=9, .melee_strength=5,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=4, .charge_capacity=0, .depleted_item_id=0},
    {.id=1387, .name="Staff of fire", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=1, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=10, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,10,0}, .melee_attack=9, .melee_strength=6,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=8, .charge_capacity=0, .depleted_item_id=0},
    {.id=22296, .name="Staff of light", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=75, .defence_level=0, .hitpoints_level=0,
     .magic_attack=17, .magic_damage_permille=150, .prayer=0,
     .defence={0,3,3,17,0}, .melee_attack=0, .melee_strength=72,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=309, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=11791, .name="Staff of the dead", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=75, .defence_level=0, .hitpoints_level=0,
     .magic_attack=17, .magic_damage_permille=150, .prayer=0,
     .defence={0,3,3,17,0}, .melee_attack=0, .melee_strength=72,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=405, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=1383, .name="Staff of water", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=1, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=10, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,10,0}, .melee_attack=7, .melee_strength=3,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=1, .infinite_runes=2, .charge_capacity=0, .depleted_item_id=0},
    {.id=22555, .name="Thammaron's sceptre", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=60, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=17000, .depleted_item_id=22552, .powered_divisor=3,
     .powered_multiplier=1, .powered_offset=-24},
    {.id=22552, .name="Thammaron's sceptre (u)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=60, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=17000, .depleted_item_id=22552, .powered_divisor=0,
     .powered_multiplier=1, .powered_offset=-24},
    {.id=30064, .name="Tome of earth", .slot=FC_EQUIP_SLOT_SHIELD,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,8,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=30066},
    {.id=30066, .name="Tome of earth (empty)", .slot=FC_EQUIP_SLOT_SHIELD,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,8,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=20714, .name="Tome of fire", .slot=FC_EQUIP_SLOT_SHIELD,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,8,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=20716},
    {.id=20716, .name="Tome of fire (empty)", .slot=FC_EQUIP_SLOT_SHIELD,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,8,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=25574, .name="Tome of water", .slot=FC_EQUIP_SLOT_SHIELD,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,8,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=25576},
    {.id=25576, .name="Tome of water (empty)", .slot=FC_EQUIP_SLOT_SHIELD,
     .magic_level=50, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=0, .prayer=0,
     .defence={0,0,0,8,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=19544, .name="Tormented bracelet", .slot=FC_EQUIP_SLOT_HANDS,
     .magic_level=0, .attack_level=0, .defence_level=0, .hitpoints_level=75,
     .magic_attack=10, .magic_damage_permille=50, .prayer=2,
     .defence={0,0,0,0,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=11905, .name="Trident of the seas (full)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=11908, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-15},
    {.id=11907, .name="Trident of the seas", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=11908, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-15},
    {.id=11908, .name="Uncharged trident", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=11908, .powered_divisor=0, .powered_multiplier=1, .powered_offset=-15},
    {.id=22288, .name="Trident of the seas (e)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=22290, .powered_divisor=3,
     .powered_multiplier=1, .powered_offset=-15},
    {.id=22290, .name="Uncharged trident (e)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=22290, .powered_divisor=0,
     .powered_multiplier=1, .powered_offset=-15},
    {.id=33326, .name="Trident of the seas (e) (o)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=33328, .powered_divisor=3,
     .powered_multiplier=1, .powered_offset=-15},
    {.id=33328, .name="Uncharged trident (e) (o)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=33328, .powered_divisor=0,
     .powered_multiplier=1, .powered_offset=-15},
    {.id=33323, .name="Trident of the seas (full) (o)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=33322, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-15},
    {.id=33322, .name="Trident of the seas (o)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=15, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=33322, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-15},
    {.id=12899, .name="Trident of the swamp", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=12900, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-6},
    {.id=12900, .name="Uncharged toxic trident", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=12900, .powered_divisor=0, .powered_multiplier=1, .powered_offset=-6},
    {.id=22292, .name="Trident of the swamp (e)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=22294, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-6},
    {.id=22294, .name="Uncharged toxic trident (e)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=75, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=22294, .powered_divisor=0, .powered_multiplier=1, .powered_offset=-6},
    {.id=33318, .name="Trident of the swamp (e) (o)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=78, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=33320, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-6},
    {.id=33320, .name="Uncharged toxic trident (e)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=78, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=33320, .powered_divisor=0, .powered_multiplier=1, .powered_offset=-6},
    {.id=33314, .name="Trident of the swamp (o)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=78, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=33316, .powered_divisor=3, .powered_multiplier=1, .powered_offset=-6},
    {.id=33316, .name="Uncharged toxic trident (o)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=78, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=25, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,15,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=2500, .depleted_item_id=33316, .powered_divisor=0, .powered_multiplier=1, .powered_offset=-6},
    {.id=27275, .name="Tumeken's shadow", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=85, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=35, .magic_damage_permille=0, .prayer=1,
     .defence={0,0,0,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=5, .range=8, .two_handed=1, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=27277, .powered_divisor=3, .powered_multiplier=1, .powered_offset=3},
    {.id=27277, .name="Tumeken's shadow (uncharged)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=85, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=35, .magic_damage_permille=0, .prayer=1,
     .defence={0,0,0,20,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=5, .range=8, .two_handed=1, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=27277, .powered_divisor=0, .powered_multiplier=1, .powered_offset=3},
    {.id=26241, .name="Virtus mask", .slot=FC_EQUIP_SLOT_HEAD,
     .magic_level=78, .attack_level=0, .defence_level=75, .hitpoints_level=0,
     .magic_attack=8, .magic_damage_permille=20, .prayer=1,
     .ranged_attack=-3,
     .defence={15,14,16,6,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=26245, .name="Virtus robe bottom", .slot=FC_EQUIP_SLOT_LEGS,
     .magic_level=78, .attack_level=0, .defence_level=75, .hitpoints_level=0,
     .magic_attack=26, .magic_damage_permille=20, .prayer=1,
     .ranged_attack=-9,
     .defence={31,28,34,22,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=26243, .name="Virtus robe top", .slot=FC_EQUIP_SLOT_BODY,
     .magic_level=78, .attack_level=0, .defence_level=75, .hitpoints_level=0,
     .magic_attack=35, .magic_damage_permille=20, .prayer=2,
     .ranged_attack=-11,
     .defence={47,36,56,31,0}, .melee_attack=0, .melee_strength=0,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=28585, .name="Warped sceptre", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=62, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=12, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,12,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=28583, .powered_divisor=37,
     .powered_multiplier=8, .powered_offset=96},
    {.id=28583, .name="Warped sceptre (uncharged)", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=62, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=12, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,12,0}, .melee_attack=0, .melee_strength=0,
     .weapon_kind=FC_WEAPON_POWERED_STAFF, .speed=4, .range=7, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=20000, .depleted_item_id=28583, .powered_divisor=0, .powered_multiplier=8, .powered_offset=96},
    {.id=2417, .name="Zamorak staff", .slot=FC_EQUIP_SLOT_WEAPON,
     .magic_level=60, .attack_level=0, .defence_level=0, .hitpoints_level=0,
     .magic_attack=6, .magic_damage_permille=0, .prayer=0,
     .defence={2,3,1,6,0}, .melee_attack=6, .melee_strength=2,
     .weapon_kind=FC_WEAPON_MAGIC_STAFF, .speed=5, .range=10, .two_handed=0, .visual_profile=-2,
     .autocast=0, .infinite_runes=0, .charge_capacity=0, .depleted_item_id=0},
    {.id=554, .name="Fire rune", .slot=-1, .stackable=1},
    {.id=555, .name="Water rune", .slot=-1, .stackable=1},
    {.id=556, .name="Air rune", .slot=-1, .stackable=1},
    {.id=557, .name="Earth rune", .slot=-1, .stackable=1},
    {.id=558, .name="Mind rune", .slot=-1, .stackable=1},
    {.id=559, .name="Body rune", .slot=-1, .stackable=1},
    {.id=560, .name="Death rune", .slot=-1, .stackable=1},
    {.id=561, .name="Nature rune", .slot=-1, .stackable=1},
    {.id=562, .name="Chaos rune", .slot=-1, .stackable=1},
    {.id=565, .name="Blood rune", .slot=-1, .stackable=1},
    {.id=566, .name="Soul rune", .slot=-1, .stackable=1},
    {.id=21880, .name="Wrath rune", .slot=-1, .stackable=1},
    {.id=4694, .name="Steam rune", .slot=-1, .stackable=1},
    {.id=4695, .name="Mist rune", .slot=-1, .stackable=1},
    {.id=4696, .name="Dust rune", .slot=-1, .stackable=1},
    {.id=4697, .name="Smoke rune", .slot=-1, .stackable=1},
    {.id=4698, .name="Mud rune", .slot=-1, .stackable=1},
    {.id=4699, .name="Lava rune", .slot=-1, .stackable=1},
    {.id=385, .name="Shark", .slot=-1},
    {.id=2434, .name="Prayer potion(4)", .slot=-1},
    {.id=139, .name="Prayer potion(3)", .slot=-1},
    {.id=141, .name="Prayer potion(2)", .slot=-1},
    {.id=143, .name="Prayer potion(1)", .slot=-1},
    {.id=229, .name="Vial", .slot=-1},
};

int fc_item_count(void) { return (int)(sizeof(ITEMS) / sizeof(ITEMS[0])); }

const FcItemDef *fc_item_at(int index) {
    return index >= 0 && index < fc_item_count() ? &ITEMS[index] : NULL;
}

const FcItemDef *fc_item_definition(int item_id) {
    for (unsigned i = 0; i < sizeof(ITEMS) / sizeof(ITEMS[0]); i++)
        if (ITEMS[i].id == item_id) return &ITEMS[i];
    return NULL;
}

void fc_items_recalculate(FcPlayer *p) {
    p->ranged_attack_bonus = p->ranged_strength_bonus = 0;
    p->defence_stab = p->defence_slash = p->defence_crush = 0;
    p->defence_magic = p->defence_ranged = p->prayer_bonus = 0;
    p->melee_attack_bonus = p->melee_strength_bonus = p->crystal_piece_mask = 0;
    p->magic_attack_bonus = p->magic_damage_permille = 0;
    const FcItemDef *weapon = fc_item_definition(p->equipment[FC_EQUIP_SLOT_WEAPON].item_id);
    const FcItemDef *ammo = fc_item_definition(p->equipment[FC_EQUIP_SLOT_AMMO].item_id);
    /* Quiver items may be worn with any weapon, but firing requires both the
     * correct category and a supported tier (RSMod validateArrows/Bolts). */
    int usable_ammo = weapon && ammo && ammo->ammo_kind != AMMO_NONE &&
        weapon->ammo_kind == ammo->ammo_kind &&
        ammo->ammo_tier <= weapon->ammo_tier;
    for (int i = 0; i < FC_EQUIPMENT_SLOTS; i++) {
        const FcItemDef *item = fc_item_definition(p->equipment[i].item_id);
        if (!item) continue;
        if (i == FC_EQUIP_SLOT_AMMO && item->ammo_kind && !usable_ammo)
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
        if (weapon && weapon->melee_type == 1) p->melee_attack_bonus += item->melee_stab;
        if (weapon && weapon->melee_type == 2) p->melee_attack_bonus += item->melee_slash;
        p->melee_strength_bonus += item->melee_strength;
        p->crystal_piece_mask |= item->crystal_piece;
        p->magic_attack_bonus += item->magic_attack;
        p->magic_damage_permille += item->magic_damage_permille;
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
    const FcSpellDef *spell = fc_spell_definition(p->autocast_spell);
    if (spell && (!weapon || !(weapon->autocast & spell->autocast))) p->autocast_spell = 0;
    if (p->equipment[FC_EQUIP_SLOT_HANDS].item_id != 31106 || (weapon && weapon->two_handed))
        p->confliction_missed = 0;
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

static const int LOADOUT_RUNES[] = {
    554,555,556,557,558,559,560,561,562,563,564,
    565,566,9075,21880,4694,4695,4696,4697,4698,4699
};

static void reset_supplies(FcPlayer *p, int sharks, int doses) {
    if (sharks < 0) sharks = 0;
    if (sharks > FC_MAX_SHARKS) sharks = FC_MAX_SHARKS;
    if (doses < 0) doses = 0;
    if (doses > FC_MAX_PRAYER_DOSES) doses = FC_MAX_PRAYER_DOSES;
    int rune_slots = (p->infinite_resources & FC_RESOURCE_RUNES) ?
        (int)(sizeof(LOADOUT_RUNES) / sizeof(LOADOUT_RUNES[0])) : 0;
    if (sharks + (doses + 3) / 4 + rune_slots > FC_INVENTORY_SLOTS) {
        fprintf(stderr, "Loadout inventory exceeds 28 slots: %d food, %d potion slots, %d runes. Reduce initial supplies.\n",
                sharks, (doses + 3) / 4, rune_slots);
        abort();
    }
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
    if (state && state->tick == 0) {
        reset_supplies(&state->player, sharks, prayer_doses);
        if (state->player.infinite_resources & FC_RESOURCE_RUNES) {
            /* Reset supplies must not erase a magic preset's rune inventory. */
            if (fc_inventory_add_runes(state) != FC_ITEM_OK) abort();
        }
    }
}

void fc_items_init(FcPlayer *p, const FcLoadout *loadout) {
    p->infinite_resources = FC_RESOURCE_AMMO | FC_RESOURCE_CHARGES |
        (loadout->autocast_spell ? FC_RESOURCE_RUNES : 0);
    p->spellbook = FC_BOOK_STANDARD;
    p->autocast_spell = loadout->autocast_spell;
    p->manual_spell = p->magic_error = 0;
    p->confliction_missed = p->confliction_target_spawn = p->confliction_spell = 0;
    memset(p->equipment, 0, sizeof(p->equipment));
    for (int i = 0; i < loadout->equipment_count; i++) {
        const FcLoadoutEquipmentItem *item = &loadout->equipment[i];
        if (item->item_id == 810) continue; /* darts are loaded in the blowpipe */
        const FcItemDef *def = fc_item_definition((int)item->item_id);
        if (!def) { fprintf(stderr, "Unknown loadout item %u\n", item->item_id); abort(); }
        p->equipment[item->slot] = (FcItemStack){
            (int)item->item_id, def->stackable ? loadout->ammo : 1,
            item->item_id == 12926 ? loadout->ammo : def->charge_capacity
        };
    }
    fc_items_recalculate(p);
    reset_supplies(p, loadout->autocast_spell ? 0 : FC_MAX_SHARKS,
                     loadout->autocast_spell ? 0 : FC_MAX_PRAYER_DOSES);
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

FcItemResult fc_inventory_add_runes(FcState *state) {
    if (!can_change_items(state)) return FC_ITEM_BUSY;
    FcItemStack inventory[FC_INVENTORY_SLOTS];
    memcpy(inventory, state->player.inventory, sizeof(inventory));
    for (unsigned r = 0; r < sizeof(LOADOUT_RUNES)/sizeof(LOADOUT_RUNES[0]); r++) {
        int found = 0;
        for (int i = 0; i < FC_INVENTORY_SLOTS; i++) {
            if (inventory[i].item_id != LOADOUT_RUNES[r]) continue;
            if (inventory[i].quantity < 10000) inventory[i].quantity = 10000;
            found = 1;
            break;
        }
        if (!found && !add_to_inventory(inventory, (FcItemStack){LOADOUT_RUNES[r],10000,0}))
            return FC_ITEM_NO_SPACE;
    }
    memcpy(state->player.inventory, inventory, sizeof(inventory));
    return FC_ITEM_OK;
}

FcItemResult fc_inventory_add(FcState *state, FcItemStack item) {
    if (!can_change_items(state)) return FC_ITEM_BUSY;
    const FcItemDef *def = fc_item_definition(item.item_id);
    if (!def || item.quantity <= 0 || (!def->stackable && item.quantity != 1) ||
        item.charges < 0 || (def->charge_capacity && item.charges > def->charge_capacity))
        return FC_ITEM_INVALID;
    if (!add_to_inventory(state->player.inventory,item)) return FC_ITEM_NO_SPACE;
    if (item.item_id == 385) state->player.sharks_remaining++;
    state->player.prayer_doses_remaining += potion_doses(item.item_id);
    return FC_ITEM_OK;
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
        p->max_hp / 10 < item->hitpoints_level || p->magic_level < item->magic_level ||
        p->attack_level < item->attack_level || p->strength_level < item->strength_level ||
        p->prayer_level < item->prayer_level) return FC_ITEM_REQUIREMENTS;
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
    fc_items_recalculate(p);
    p->manual_spell = 0;
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
    fc_items_recalculate(p);
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
    if (p->infinite_resources & FC_RESOURCE_AMMO) return;
    p->ammo_count--;
    FcItemStack *weapon = &p->equipment[FC_EQUIP_SLOT_WEAPON];
    if (weapon->item_id == 12926) weapon->charges--;
    else {
        FcItemStack *ammo = &p->equipment[FC_EQUIP_SLOT_AMMO];
        if (ammo->quantity > 0 && --ammo->quantity == 0) {
            *ammo = (FcItemStack){0};
            fc_items_recalculate(p);
        }
    }
}
