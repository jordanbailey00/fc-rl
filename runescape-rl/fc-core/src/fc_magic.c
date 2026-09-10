#include "fc_magic_internal.h"
#include "fc_api.h"
#include "fc_combat.h"
#include "fc_items_internal.h"
#include "fc_npc.h"
#include "fc_pathfinding.h"
#include <stdint.h>
#include <string.h>

/* RuneC's reviewed 57 combat spells (SPEL v2, 2026-09-08). Only static content
 * is imported; simulation state, resource transactions and hits are FC-owned.
 * No utility/teleport spell is silently treated as a combat spell. */
static const FcSpellDef SPELLS[] = {
    {30, "Wind Strike", 0, 1, 2, 1, 2,
     {{556,1},{558,1}}, 1, 1, 0, 0, 2, 0, 0, 1, 0, 0},
    {14, "Confuse", 0, 3, 0, 16, 3,
     {{557,2},{555,3},{559,1}}, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0},
    {37, "Water Strike", 0, 5, 4, 1, 3,
     {{556,1},{555,1},{558,1}}, 2, 1, 0, 0, 2, 0, 0, 1, 0, 0},
    {20, "Earth Strike", 0, 9, 6, 1, 3,
     {{556,1},{557,2},{558,1}}, 3, 1, 0, 0, 2, 0, 0, 1, 0, 0},
    {60, "Weaken", 0, 11, 0, 16, 3,
     {{557,2},{555,3},{559,1}}, 0, 0, 0, 0, 1, 5, 0, 0, 0, 0},
    {21, "Fire Strike", 0, 13, 8, 1, 3,
     {{556,2},{554,3},{558,1}}, 4, 1, 0, 0, 2, 0, 0, 1, 0, 0},
    {36, "Wind Bolt", 0, 17, 9, 1, 2,
     {{556,2},{562,1}}, 1, 2, 0, 0, 2, 0, 0, 1, 0, 0},
    {61, "Curse", 0, 19, 0, 16, 3,
     {{557,3},{555,2},{559,1}}, 0, 0, 0, 0, 2, 5, 0, 0, 0, 0},
    {62, "Bind", 0, 20, 0, 2, 3,
     {{557,3},{555,3},{561,2}}, 0, 0, 8, 0, 2, 0, 0, 0, 0, 0},
    {34, "Water Bolt", 0, 23, 10, 1, 3,
     {{556,2},{555,2},{562,1}}, 2, 2, 0, 0, 2, 0, 0, 1, 0, 0},
    {19, "Earth Bolt", 0, 29, 11, 1, 3,
     {{556,2},{557,3},{562,1}}, 3, 2, 0, 0, 2, 0, 0, 1, 0, 0},
    {2, "Fire Bolt", 0, 35, 12, 1, 3,
     {{556,3},{554,4},{562,1}}, 4, 2, 0, 0, 2, 0, 0, 1, 0, 0},
    {15, "Crumble Undead", 0, 39, 15, 1, 3,
     {{556,2},{557,2},{562,1}}, 0, 0, 0, 0, 2, 0, 0, 256, 0, 1},
    {43, "Wind Blast", 0, 41, 13, 1, 2,
     {{556,3},{560,1}}, 1, 3, 0, 0, 2, 0, 0, 1, 0, 0},
    {26, "Water Blast", 0, 47, 14, 1, 3,
     {{556,3},{555,3},{560,1}}, 2, 3, 0, 0, 2, 0, 0, 1, 0, 0},
    {11, "Iban Blast", 0, 50, 25, 1, 2,
     {{554,5},{560,1}}, 0, 3, 0, 0, 2, 0, 0, 8, 1409, 0},
    {46, "Magic Dart", 0, 50, 10, 1, 2,
     {{560,1},{558,4}}, 0, 0, 0, 0, 2, 0, 0, 16, 4170, 0},
    {63, "Snare", 0, 50, 3, 3, 3,
     {{557,4},{555,4},{561,3}}, 0, 0, 16, 0, 2, 0, 0, 0, 0, 0},
    {44, "Earth Blast", 0, 53, 15, 1, 3,
     {{556,3},{557,4},{560,1}}, 3, 3, 0, 0, 2, 0, 0, 1, 0, 0},
    {1, "Fire Blast", 0, 59, 16, 1, 3,
     {{556,4},{554,5},{560,1}}, 4, 3, 0, 0, 2, 0, 0, 1, 0, 0},
    {16, "Saradomin Strike", 0, 60, 20, 1, 3,
     {{556,4},{554,2},{565,2}}, 0, 1, 0, 0, 2, 0, 0, 32, 2415, 0},
    {23, "Flames of Zamorak", 0, 60, 20, 17, 3,
     {{556,1},{554,4},{565,2}}, 0, 0, 0, 0, 3, 10, 0, 128, 2417, 0},
    {31, "Claws of Guthix", 0, 60, 20, 17, 3,
     {{556,4},{554,1},{565,2}}, 0, 0, 0, 0, 2, 10, 0, 64, 2416, 0},
    {45, "Wind Wave", 0, 62, 17, 1, 2,
     {{556,5},{565,1}}, 1, 4, 0, 0, 2, 0, 0, 1, 0, 0},
    {35, "Water Wave", 0, 65, 18, 1, 3,
     {{556,5},{555,7},{565,1}}, 2, 4, 0, 0, 2, 0, 0, 1, 0, 0},
    {64, "Vulnerability", 0, 66, 0, 16, 3,
     {{557,5},{555,5},{566,1}}, 0, 0, 0, 0, 2, 10, 0, 0, 0, 0},
    {13, "Earth Wave", 0, 70, 19, 1, 3,
     {{556,5},{557,7},{565,1}}, 3, 4, 0, 0, 2, 0, 0, 1, 0, 0},
    {65, "Enfeeble", 0, 73, 0, 16, 3,
     {{557,8},{555,8},{566,1}}, 0, 0, 0, 0, 1, 10, 0, 0, 0, 0},
    {3, "Fire Wave", 0, 75, 20, 1, 3,
     {{556,5},{554,7},{565,1}}, 4, 4, 0, 0, 2, 0, 0, 1, 0, 0},
    {66, "Entangle", 0, 79, 5, 3, 3,
     {{557,5},{555,5},{561,4}}, 0, 0, 24, 0, 2, 0, 0, 0, 0, 0},
    {67, "Stun", 0, 80, 0, 16, 3,
     {{557,12},{555,12},{566,1}}, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0},
    {158, "Wind Surge", 0, 81, 21, 1, 2,
     {{556,7},{21880,1}}, 1, 5, 0, 0, 2, 0, 0, 1, 0, 0},
    {159, "Water Surge", 0, 85, 22, 1, 3,
     {{556,7},{555,10},{21880,1}}, 2, 5, 0, 0, 2, 0, 0, 1, 0, 0},
    {160, "Earth Surge", 0, 90, 23, 1, 3,
     {{556,7},{557,10},{21880,1}}, 3, 5, 0, 0, 2, 0, 0, 1, 0, 0},
    {161, "Fire Surge", 0, 95, 24, 1, 3,
     {{556,7},{554,10},{21880,1}}, 4, 5, 0, 0, 2, 0, 0, 1, 0, 0},
    {28, "Smoke Rush", 1, 50, 13, 9, 4,
     {{556,1},{554,1},{562,2},{560,2}}, 0, 0, 0, 2, 2, 0, 0, 2, 0, 0},
    {29, "Shadow Rush", 1, 52, 14, 17, 4,
     {{556,1},{562,2},{560,2},{566,1}}, 0, 0, 0, 0, 0, 10, 0, 2, 0, 0},
    {88, "Blood Rush", 1, 56, 15, 5, 3,
     {{565,1},{562,2},{560,2}}, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0},
    {7, "Ice Rush", 1, 58, 16, 3, 3,
     {{555,2},{562,2},{560,2}}, 0, 0, 8, 0, 2, 0, 0, 2, 0, 0},
    {96, "Smoke Burst", 1, 62, 17, 9, 4,
     {{556,2},{554,2},{562,4},{560,2}}, 0, 0, 0, 2, 2, 0, 1, 2, 0, 0},
    {119, "Shadow Burst", 1, 64, 18, 17, 4,
     {{556,1},{562,4},{560,2},{566,2}}, 0, 0, 0, 0, 0, 10, 1, 2, 0, 0},
    {85, "Blood Burst", 1, 68, 21, 5, 3,
     {{565,2},{562,4},{560,2}}, 0, 0, 0, 0, 2, 0, 1, 2, 0, 0},
    {6, "Ice Burst", 1, 70, 22, 3, 3,
     {{555,4},{562,4},{560,2}}, 0, 0, 16, 0, 2, 0, 1, 2, 0, 0},
    {121, "Smoke Blitz", 1, 74, 23, 9, 4,
     {{556,2},{554,2},{565,2},{560,2}}, 0, 0, 0, 4, 2, 0, 0, 2, 0, 0},
    {120, "Shadow Blitz", 1, 76, 24, 17, 4,
     {{556,2},{565,2},{560,2},{566,2}}, 0, 0, 0, 0, 0, 15, 0, 2, 0, 0},
    {118, "Blood Blitz", 1, 80, 25, 5, 2,
     {{565,4},{560,2}}, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0},
    {5, "Ice Blitz", 1, 82, 26, 3, 3,
     {{555,3},{565,2},{560,2}}, 0, 0, 24, 0, 2, 0, 0, 2, 0, 0},
    {82, "Smoke Barrage", 1, 86, 27, 9, 4,
     {{556,4},{554,4},{565,2},{560,4}}, 0, 0, 0, 4, 2, 0, 1, 2, 0, 0},
    {97, "Shadow Barrage", 1, 88, 28, 17, 4,
     {{556,4},{565,2},{560,4},{566,3}}, 0, 0, 0, 0, 0, 15, 1, 2, 0, 0},
    {58, "Blood Barrage", 1, 92, 29, 5, 3,
     {{565,4},{560,4},{566,1}}, 0, 0, 0, 0, 2, 0, 1, 2, 0, 0},
    {4, "Ice Barrage", 1, 94, 30, 3, 3,
     {{555,6},{565,2},{560,4}}, 0, 0, 32, 0, 2, 0, 1, 2, 0, 0},
    {166, "Ghostly Grasp", 3, 35, 12, 3, 2,
     {{556,4},{562,1}}, 0, 0, 1, 0, 2, 0, 0, 4, 0, 0},
    {169, "Inferior Demonbane", 3, 44, 16, 1, 2,
     {{554,3},{562,1}}, 0, 0, 0, 0, 2, 0, 0, 4, 0, 2},
    {167, "Skeletal Grasp", 3, 56, 17, 3, 2,
     {{557,8},{560,1}}, 0, 0, 2, 0, 2, 0, 0, 4, 0, 0},
    {170, "Superior Demonbane", 3, 62, 23, 1, 2,
     {{554,5},{566,1}}, 0, 0, 0, 0, 2, 0, 0, 4, 0, 2},
    {168, "Undead Grasp", 3, 79, 24, 3, 2,
     {{554,12},{565,1}}, 0, 0, 4, 0, 2, 0, 0, 4, 0, 0},
    {171, "Dark Demonbane", 3, 82, 30, 1, 2,
     {{554,7},{566,2}}, 0, 0, 0, 0, 2, 0, 0, 4, 0, 2},
};

int fc_spell_count(void) { return (int)(sizeof(SPELLS) / sizeof(SPELLS[0])); }
const FcSpellDef *fc_spell_at(int i) {
    return i >= 0 && i < fc_spell_count() ? &SPELLS[i] : NULL;
}
const FcSpellDef *fc_spell_definition(int id) {
    for (int i = 0; i < fc_spell_count(); i++)
        if (SPELLS[i].id == id) return &SPELLS[i];
    return NULL;
}
const char *fc_spellbook_name(int book) {
    static const char *names[] = {"Standard", "Ancient", "Lunar", "Arceuus"};
    return book >= 0 && book < FC_BOOK_COUNT ? names[book] : "Invalid";
}
const char *fc_magic_result_message(FcMagicResult result) {
    static const char *messages[] = {"", "That combat spell is not available.",
        "Your Magic level is too low.", "Select the spell's spellbook first.",
        "Your weapon cannot cast/autocast this spell.", "You don't have enough runes.",
        "Your weapon has no valid charges.", "This spell cannot affect that target.",
        "You cannot cast right now.", "Too many attacks are already in flight.",
        "That target is already affected by this curse."};
    return result >= FC_MAGIC_OK && result <= FC_MAGIC_ALREADY_DRAINED
        ? messages[result] : messages[FC_MAGIC_INVALID];
}
static const FcItemDef *held_item(const FcPlayer *p) {
    return fc_item_definition(p->equipment[FC_EQUIP_SLOT_WEAPON].item_id);
}
static int worn(const FcPlayer *p, int slot, int id) {
    return p->equipment[slot].item_id == id;
}
static int tome_element(const FcPlayer *p) {
    const FcItemStack *shield = &p->equipment[FC_EQUIP_SLOT_SHIELD];
    if (shield->charges <= 0) return 0;
    switch (shield->item_id) {
        case 20714: return 4;
        case 25574: return 2;
        case 30064: return 3;
        default: return 0;
    }
}

/* Rune costs are planned on a private inventory before committing anything.
 * Normal runes are preferred; each combination rune can satisfy both elements
 * at once. Staff/tome substitutions do not turn an empty tome into free runes. */
static int pay_runes(const FcPlayer *p, const FcSpellDef *spell,
                      FcItemStack inventory[FC_INVENTORY_SLOTS]) {
    int need[4] = {0};
    const int elemental[] = {556,555,557,554};
    const FcItemDef *weapon = held_item(p);
    int infinite = weapon ? weapon->infinite_runes : 0;
    int tome = tome_element(p);
    if (tome) infinite |= 1 << (tome - 1);
    memcpy(inventory, p->inventory, sizeof(p->inventory));
    for (int r = 0; r < spell->rune_count; r++) {
        int id = spell->runes[r][0], remaining = spell->runes[r][1];
        int element = -1;
        for (int e = 0; e < 4; e++) if (id == elemental[e]) element = e;
        if (element >= 0 && (infinite & (1 << element))) continue;
        for (int i = 0; i < FC_INVENTORY_SLOTS && remaining; i++) {
            FcItemStack *item = &inventory[i];
            if (item->item_id != id) continue;
            int take = item->quantity < remaining ? item->quantity : remaining;
            item->quantity -= take;
            remaining -= take;
            if (!item->quantity) *item = (FcItemStack){0};
        }
        if (element < 0 && remaining) return 0;
        if (element >= 0) need[element] = remaining;
    }
    const int combo[][2] = {{4695,3},{4696,5},{4697,9},{4698,6},{4699,12},{4694,10}};
    for (int c = 0; c < 6; c++) {
        int amount = 0;
        for (int e = 0; e < 4; e++)
            if ((combo[c][1] & (1 << e)) && need[e] > amount) amount = need[e];
        for (int i = 0; i < FC_INVENTORY_SLOTS && amount; i++) {
            FcItemStack *item = &inventory[i];
            if (item->item_id != combo[c][0]) continue;
            int take = item->quantity < amount ? item->quantity : amount;
            for (int e = 0; e < 4; e++) if (combo[c][1] & (1 << e)) {
                need[e] -= take;
                if (need[e] < 0) need[e] = 0;
            }
            amount -= take;
            item->quantity -= take;
            if (!item->quantity) *item = (FcItemStack){0};
        }
    }
    return !(need[0] || need[1] || need[2] || need[3]);
}

FcMagicResult fc_magic_check(const FcState *s, int id, int autocast) {
    if (!s || s->terminal || s->player.current_hp <= 0) return FC_MAGIC_BUSY;
    const FcPlayer *p = &s->player;
    const FcItemDef *weapon = held_item(p);
    const FcSpellDef *spell = fc_spell_definition(id);
    const FcItemStack *held = &p->equipment[FC_EQUIP_SLOT_WEAPON];
    if (!id) {
        if (!weapon || weapon->weapon_kind != FC_WEAPON_POWERED_STAFF)
            return FC_MAGIC_WEAPON;
        if (!weapon->powered_divisor) return FC_MAGIC_CHARGES;
        if (weapon->id == 22516) return FC_MAGIC_TARGET; /* Dawnbringer is ToB-only. */
        if (p->magic_level < weapon->magic_level) return FC_MAGIC_LEVEL;
    } else {
        if (!spell) return FC_MAGIC_INVALID;
        if (p->spellbook != spell->book) return FC_MAGIC_BOOK;
        if (p->magic_level < spell->level) return FC_MAGIC_LEVEL;
        if (autocast && (!weapon || !(weapon->autocast & spell->autocast)))
            return FC_MAGIC_WEAPON;
        if (spell->required_weapon && held->item_id != spell->required_weapon &&
            (!weapon || !(weapon->autocast & spell->autocast))) return FC_MAGIC_WEAPON;
        FcItemStack inventory[FC_INVENTORY_SLOTS];
        if (!pay_runes(p, spell, inventory)) return FC_MAGIC_RUNES;
    }
    if (weapon && weapon->charge_capacity && (!id || spell->required_weapon == 1409) &&
        (held->charges <= 0 || held->charges > weapon->charge_capacity)) return FC_MAGIC_CHARGES;
    return FC_MAGIC_OK;
}

FcMagicResult fc_set_spellbook(FcState *s, int book) {
    if (!s || s->terminal) return FC_MAGIC_BUSY;
    if (book < 0 || book >= FC_BOOK_COUNT) return FC_MAGIC_BOOK;
    s->player.spellbook = book;
    s->player.manual_spell = s->player.autocast_spell = 0;
    s->player.magic_error = 0;
    s->player.attack_target_idx = -1;
    s->player.approach_target = 0;
    return FC_MAGIC_OK;
}

FcMagicResult fc_set_autocast(FcState *s, int id) {
    if (!s || s->terminal) return FC_MAGIC_BUSY;
    if (id) {
        FcMagicResult result = fc_magic_check(s, id, 1);
        if (result != FC_MAGIC_OK) return result;
    }
    s->player.autocast_spell = id;
    s->player.manual_spell = 0;
    s->player.magic_error = 0;
    return FC_MAGIC_OK;
}

FcMagicResult fc_cast_spell(FcState *s, int id, int npc_slot) {
    if (!id) return FC_MAGIC_INVALID;
    FcMagicResult result = fc_magic_check(s, id, 0);
    if (result != FC_MAGIC_OK) return result;
    if (npc_slot < 0 || npc_slot >= FC_MAX_NPCS || !s->npcs[npc_slot].active ||
        s->npcs[npc_slot].is_dead || fc_spell_definition(id)->target_only)
        return FC_MAGIC_TARGET;
    s->player.manual_spell = id;
    s->player.attack_target_idx = npc_slot;
    s->player.approach_target = 1;
    s->player.approach_target_size = 0;
    s->player.magic_error = 0;
    return FC_MAGIC_OK;
}

int fc_magic_active(const FcPlayer *p) {
    return p->manual_spell || p->autocast_spell ||
        p->weapon_kind == FC_WEAPON_POWERED_STAFF;
}
int fc_magic_attack_range(const FcPlayer *p) {
    return p->manual_spell || p->autocast_spell ? 10 : p->weapon_range;
}
static int powered_base(const FcPlayer *p) {
    const FcItemDef *weapon = held_item(p);
    if (!weapon || !weapon->powered_divisor) return 0;
    int hit = (p->magic_level * weapon->powered_multiplier + weapon->powered_offset) /
        weapon->powered_divisor;
    return hit > 0 ? hit : 1;
}
static int smoke_staff(const FcPlayer *p) {
    int id = p->equipment[FC_EQUIP_SLOT_WEAPON].item_id;
    return id == 11998 || id == 12000 || id == 30634;
}
static int shadow(const FcPlayer *p, int spell_id) {
    int id = p->equipment[FC_EQUIP_SLOT_WEAPON].item_id;
    return !spell_id && (id == 27275 || id == 28547);
}
static int spell_base(const FcPlayer *p, const FcSpellDef *spell) {
    if (!spell) return powered_base(p);
    if (spell->required_weapon == 4170) return 10 + p->magic_level / 10;
    int hit = spell->max_hit;
    if (spell->element && spell->tier) {
        const int levels[5][4] = {{1,5,9,13},{17,23,29,35},{41,47,53,59},
                                  {62,65,70,75},{81,85,90,95}};
        const int hits[5][4] = {{2,4,6,8},{9,10,11,12},{13,14,15,16},
                                {17,18,19,20},{21,22,23,24}};
        for (int e = 0; e < 4; e++)
            if (p->magic_level >= levels[spell->tier-1][e]) hit = hits[spell->tier-1][e];
        if (spell->tier == 2 && worn(p, FC_EQUIP_SLOT_HANDS, 777)) hit += 3;
    }
    return hit;
}
static int magic_pre_tome_max_hit(const FcPlayer *p, const FcNpc *target, int id) {
    const FcSpellDef *spell = fc_spell_definition(id);
    if (id && !spell) return 0;
    int base = spell_base(p, spell);
    int bonus = p->magic_damage_permille;
    if (shadow(p, id)) { bonus *= 3; if (bonus > 1000) bonus = 1000; }
    if (spell && spell->book == FC_BOOK_STANDARD && smoke_staff(p)) bonus += 100;
    if (spell && spell->book == FC_BOOK_ANCIENT) {
        const int slots[] = {FC_EQUIP_SLOT_HEAD,FC_EQUIP_SLOT_BODY,FC_EQUIP_SLOT_LEGS};
        const int ids[] = {26241,26243,26245};
        for (int i = 0; i < 3; i++) if (worn(p,slots[i],ids[i])) bonus += 30;
    }
    int maximum = base * (1000 + bonus) / 1000;
    if (spell && spell->element == 2 && target)
        maximum += base * fc_npc_get_stats(target->npc_type)->water_weakness_pct / 100;
    return maximum > 0 ? maximum : 0;
}
int fc_magic_max_hit(const FcPlayer *p, const FcNpc *target, int id) {
    const FcSpellDef *spell = fc_spell_definition(id);
    int maximum = magic_pre_tome_max_hit(p,target,id);
    return spell && spell->element && tome_element(p) == spell->element
        ? maximum * 11 / 10 : maximum;
}
static int water_tome_curse(const FcPlayer *p, const FcSpellDef *spell) {
    return spell && spell->book == FC_BOOK_STANDARD &&
        (spell->freeze_ticks || spell->drain_pct) && tome_element(p) == 2;
}
int fc_magic_attack_roll(const FcPlayer *p, const FcNpc *target, int id) {
    const FcSpellDef *spell = fc_spell_definition(id);
    int bonus = p->magic_attack_bonus * (shadow(p,id) ? 3 : 1);
    if (bonus < -64) bonus = -64;
    /* Manual/autocast has +9; the powered staff's accurate stance adds +2. */
    int base = (p->magic_level + (id ? 9 : 11)) * (bonus + 64);
    int roll = base;
    if (spell && spell->book == FC_BOOK_STANDARD && smoke_staff(p)) roll = roll * 11 / 10;
    if (water_tome_curse(p,spell)) roll = roll * 6 / 5;
    else if (spell && tome_element(p) == 2 && spell->element == 2) roll = roll * 11 / 10;
    if (spell && spell->element == 2 && target)
        roll += base * fc_npc_get_stats(target->npc_type)->water_weakness_pct / 100;
    return roll;
}

FcMagicResult fc_magic_launch(FcState *s, FcNpc *target, int distance) {
    FcPlayer *p = &s->player;
    int id = p->manual_spell ? p->manual_spell : p->autocast_spell;
    const FcSpellDef *spell = fc_spell_definition(id);
    const FcItemDef *weapon = held_item(p);
    /* Twinflame adapts autocast only, and only when the replacement is legal.
     * Fight Caves' elemental weakness is water; no client-side spell decision. */
    if (weapon && weapon->id == 30634 && !p->manual_spell && spell && spell->element &&
        spell->element != 2 && fc_npc_get_stats(target->npc_type)->water_weakness_pct) {
        for (int i = 0; i < fc_spell_count(); i++) {
            const FcSpellDef *water = fc_spell_at(i);
            if (water->element == 2 && water->tier == spell->tier &&
                fc_magic_check(s, water->id, 1) == FC_MAGIC_OK) {
                id = water->id;
                spell = water;
                break;
            }
        }
    }
    FcMagicResult result = fc_magic_check(s, id, !p->manual_spell);
    if (result != FC_MAGIC_OK) return result;
    if (spell && spell->target_only) return FC_MAGIC_TARGET;
    if (spell && spell->max_hit == 0 && spell->drain_pct &&
        target->stat_drain[spell->drain_stat]) return FC_MAGIC_ALREADY_DRAINED;
    int twin = weapon && weapon->id == 30634 && spell && spell->element &&
        spell->tier >= 2 && spell->tier <= 4;
    if (target->num_pending_hits + 1 + twin > FC_MAX_PENDING_HITS) return FC_MAGIC_QUEUE;

    int targets[9], count = 1;
    targets[0] = (int)(target - s->npcs);
    if (spell && spell->area) {
        /* Fight Caves is multi-combat. Select once, primary first then stable
         * NPC-slot order; footprints intersect the 3x3 around the primary SW tile. */
        for (int i = 0; i < FC_MAX_NPCS && count < 9; i++) {
            FcNpc *n = &s->npcs[i];
            if (n == target || !n->active || n->is_dead ||
                n->num_pending_hits >= FC_MAX_PENDING_HITS) continue;
            if (fc_distance_between_areas(target->x,target->y,1,n->x,n->y,n->size) > 1 ||
                !fc_has_los_between_areas(p->x,p->y,1,n->x,n->y,n->size,s->los_flags)) continue;
            targets[count++] = i;
        }
    }
    if (spell) {
        FcItemStack inventory[FC_INVENTORY_SLOTS];
        if (!pay_runes(p,spell,inventory)) return FC_MAGIC_RUNES;
        int save = (p->infinite_resources & FC_RESOURCE_RUNES) || (weapon && (weapon->id == 21006
            ? fc_rng_int(s,100) < 15
            : (weapon->id == 11791 || weapon->id == 22296) && fc_rng_int(s,8) == 0));
        if (!save) memcpy(p->inventory,inventory,sizeof(inventory));
    }
    /* Capture all calculations while launch equipment is still present. */
    int weapon_id = weapon ? weapon->id : 0;
    /* Alter's server delay is 2+floor((1+d)/3). FC also consumes the queue
     * once during this launch tick, so add one to preserve that boundary. */
    int delay = 3 + (1 + distance) / 3;
    FcRenderEvents *e = &s->render_events;
    e->player_attack_spell_id = id;
    e->player_attack_weapon_id = weapon_id;
    e->player_magic_target_count = count;
    for (int t = 0; t < count; t++) {
        FcNpc *n = &s->npcs[targets[t]];
        const FcNpcStats *stats = fc_npc_get_stats(n->npc_type);
        int defence = stats->magic_level - n->stat_drain[FC_MAGIC_DRAIN_MAGIC];
        int attack_roll = fc_magic_attack_roll(p,n,id);
        int defence_roll = fc_npc_def_roll(defence,stats->magic_def_bonus);
        int confliction = t == 0 && worn(p,FC_EQUIP_SLOT_HANDS,31106) &&
            (!weapon || !weapon->two_handed);
        int spell_key = id ? id : -weapon_id;
        int reroll = confliction && p->confliction_missed &&
            p->confliction_target_spawn == n->spawn_index && p->confliction_spell == spell_key;
        int accurate = fc_rng_float(s) < (reroll
            ? fc_double_attack_hit_chance(attack_roll,defence_roll)
            : fc_hit_chance(attack_roll,defence_roll));
        if (t == 0) {
            p->confliction_missed = confliction && !accurate;
            p->confliction_target_spawn = confliction ? n->spawn_index : 0;
            p->confliction_spell = confliction ? spell_key : 0;
        }
        int maximum = magic_pre_tome_max_hit(p,n,id);
        int damage = accurate ? fc_roll_player_damage_tenths(s,maximum) : 0;
        /* Tomes multiply the rolled whole-HP hit, not the range sampled by RNG. */
        if (spell && spell->element && tome_element(p) == spell->element)
            damage = (damage / 10 * 11 / 10) * 10;
        int sang = !id && accurate && (weapon_id == 22323 || weapon_id == 25731) &&
            fc_rng_int(s,5) == 0;
        if (sang) damage += 80; /* July 2026 passive: +8 HP on a healing proc. */
        fc_queue_pending_hit(n->pending_hits,&n->num_pending_hits,FC_MAX_PENDING_HITS,
            damage,delay,ATTACK_MAGIC,-1,0);
        FcPendingHit *hit = &n->pending_hits[n->num_pending_hits-1];
        hit->spell_id = id;
        hit->accurate = accurate;
        hit->magic_curse_boost = water_tome_curse(p,spell);
        if (sang) hit->magic_heal_divisor = 2;
        if (!id && accurate &&
            (weapon_id == 12899 || weapon_id == 22292 ||
             weapon_id == 33314 || weapon_id == 33318) && fc_rng_int(s,4) == 0)
            hit->magic_poison = n->npc_type == NPC_TZTOK_JAD ? 6 : -6;
        e->player_magic_targets[t] = targets[t];
        e->player_magic_accurate[t] = accurate;
        if (twin) {
            fc_queue_pending_hit(n->pending_hits,&n->num_pending_hits,FC_MAX_PENDING_HITS,
                (damage / 10 * 2 / 5) * 10,delay + 1,ATTACK_MAGIC,-1,0);
            FcPendingHit *echo = &n->pending_hits[n->num_pending_hits-1];
            echo->spell_id = id;
            echo->accurate = accurate;
            e->player_magic_targets[count] = targets[t];
            e->player_magic_accurate[count] = accurate;
            e->player_magic_delay_offset[count] = 1;
            e->player_magic_target_count++;
        }
        s->ep_attack_cycles_to_npc_type[n->npc_type]++;
    }
    e->player_attack_fired = s->attack_attempt_this_tick = 1;
    if (spell && ((spell->element && tome_element(p) == spell->element) ||
                 water_tome_curse(p,spell))) {
        FcItemStack *tome = &p->equipment[FC_EQUIP_SLOT_SHIELD];
        const FcItemDef *def = fc_item_definition(tome->item_id);
        if (!(p->infinite_resources & FC_RESOURCE_CHARGES) && --tome->charges == 0)
            tome->item_id = def->depleted_item_id;
    }
    e->player_attack_source_x = p->x;
    e->player_attack_source_y = p->y;
    e->player_attack_target_npc_slot = targets[0];
    e->player_attack_target_x = target->x;
    e->player_attack_target_y = target->y;
    e->player_attack_target_size = target->size;
    e->player_attack_hit_delay_ticks = delay;
    p->attack_timer = id ? (weapon_id == 30634 ? 6 :
        weapon_id == 24423 && !p->manual_spell ? 4 : 5) : weapon->speed;
    if (weapon && weapon->charge_capacity && (!id || spell->required_weapon == 1409)) {
        FcItemStack *held = &p->equipment[FC_EQUIP_SLOT_WEAPON];
        if (!(p->infinite_resources & FC_RESOURCE_CHARGES) &&
            --held->charges == 0 && weapon->depleted_item_id) held->item_id = weapon->depleted_item_id;
        fc_items_recalculate(p);
    }
    if (p->manual_spell) {
        p->manual_spell = 0;
        p->attack_target_idx = -1;
        p->approach_target = 0;
    }
    p->hit_landed_this_tick = 1;
    return FC_MAGIC_OK;
}

void fc_magic_resolve(FcState *s, FcNpc *n, const FcPendingHit *hit, int actual) {
    if (hit->attack_style != ATTACK_MAGIC || !hit->accurate || n->is_dead) return;
    const FcSpellDef *spell = fc_spell_definition(hit->spell_id);
    int divisor = spell && (spell->effects & FC_MAGIC_HEAL) ? 4 : hit->magic_heal_divisor;
    if (divisor && s->player.current_hp > 0) {
        /* Whole hitpoints, including overkill capping, before converting back. */
        int heal = (actual / 10 / divisor) * 10;
        s->player.current_hp += heal;
        if (s->player.current_hp > s->player.max_hp) s->player.current_hp = s->player.max_hp;
    }
    if (n->current_hp <= 0) return;
    if (hit->magic_poison < 0 && n->poison_severity >= 0) {
        n->poison_severity = hit->magic_poison;
        n->poison_next_tick = s->tick + 30;
    }
    int poison = hit->magic_poison > 0 ? hit->magic_poison :
        spell ? spell->poison_damage : 0;
    if (poison && n->poison_severity >= 0) {
        int severity = (poison - 1) * 5 + 1;
        if (severity > n->poison_severity) {
            n->poison_severity = severity;
            if (!n->poison_next_tick) n->poison_next_tick = s->tick + 30;
        }
    }
    if (!spell) return;
    if (spell->freeze_ticks && s->tick >= n->freeze_immune_until &&
        (spell->book != FC_BOOK_ARCEUUS || fc_rng_int(s,2) == 0)) {
        n->frozen_until = s->tick + spell->freeze_ticks + 1;
        n->freeze_immune_until = n->frozen_until + 5;
    }
    if (spell->drain_pct) {
        const FcNpcStats *stats = fc_npc_get_stats(n->npc_type);
        int base = spell->drain_stat == FC_MAGIC_DRAIN_ATTACK ? stats->att_level :
            spell->drain_stat == FC_MAGIC_DRAIN_DEFENCE ? stats->def_level :
            spell->drain_stat == FC_MAGIC_DRAIN_MAGIC ? stats->magic_level :
            n->npc_type == NPC_YT_HURKOT ? 100 : stats->ranged_level;
        int percent = spell->drain_pct * (hit->magic_curse_boost ? 3 : 2) / 2;
        int drain = base - base * (100-percent) / 100;
        if (drain > n->stat_drain[spell->drain_stat]) n->stat_drain[spell->drain_stat] = drain;
        if (!n->stat_restore_tick) n->stat_restore_tick = s->tick + 100;
    }
}
void fc_magic_tick_npc(FcState *s, FcNpc *n) {
    if (n->poison_severity && s->tick >= n->poison_next_tick &&
        fc_queue_pending_hit(n->pending_hits,&n->num_pending_hits,FC_MAX_PENDING_HITS,
            (n->poison_severity < 0 ? -n->poison_severity :
                (n->poison_severity+4)/5)*10,1,ATTACK_NONE,-1,0)) {
        if (n->poison_severity > 0) n->poison_severity--;
        else if (n->poison_severity > -20) n->poison_severity -= 2;
        n->poison_next_tick = n->poison_severity ? s->tick+30 : 0;
    }
    if (n->stat_restore_tick && s->tick >= n->stat_restore_tick) {
        int remaining = 0;
        for (int i = 0; i < 4; i++) {
            if (n->stat_drain[i] > 0) n->stat_drain[i]--;
            remaining |= n->stat_drain[i];
        }
        n->stat_restore_tick = remaining ? s->tick+100 : 0;
    }
}
