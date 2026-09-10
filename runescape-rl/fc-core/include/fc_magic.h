#ifndef FC_MAGIC_H
#define FC_MAGIC_H

#include "fc_types.h"

enum { FC_BOOK_STANDARD, FC_BOOK_ANCIENT, FC_BOOK_LUNAR, FC_BOOK_ARCEUUS,
       FC_BOOK_COUNT };
enum { FC_RUNE_AIR=1, FC_RUNE_WATER=2, FC_RUNE_EARTH=4, FC_RUNE_FIRE=8 };
enum { FC_AUTOCAST_STANDARD=1, FC_AUTOCAST_ANCIENT=2, FC_AUTOCAST_ARCEUUS=4,
       FC_AUTOCAST_IBAN=8, FC_AUTOCAST_DART=16, FC_AUTOCAST_SARADOMIN=32,
       FC_AUTOCAST_GUTHIX=64, FC_AUTOCAST_ZAMORAK=128, FC_AUTOCAST_UNDEAD=256 };
enum { FC_MAGIC_DAMAGE=1, FC_MAGIC_FREEZE=2, FC_MAGIC_HEAL=4,
       FC_MAGIC_POISON=8, FC_MAGIC_DRAIN=16 };
enum { FC_MAGIC_DRAIN_ATTACK, FC_MAGIC_DRAIN_STRENGTH,
       FC_MAGIC_DRAIN_DEFENCE, FC_MAGIC_DRAIN_MAGIC };

typedef struct {
    int id; /* stable RuneC spell index + 1; zero means no spell */
    const char *name;
    int book, level, max_hit, effects;
    int rune_count;
    int runes[4][2];
    int element, tier; /* element 1..4 = air/water/earth/fire; tier 1..5 */
    int freeze_ticks, poison_damage, drain_stat, drain_pct;
    int area; /* burst/barrage: primary + up to eight nearby NPCs */
    int autocast;
    int required_weapon; /* specific god/Iban/Dart validation */
    int target_only; /* 1 undead, 2 demon; no Fight Caves NPC qualifies */
} FcSpellDef;

typedef enum {
    FC_MAGIC_OK, FC_MAGIC_INVALID, FC_MAGIC_LEVEL, FC_MAGIC_BOOK,
    FC_MAGIC_WEAPON, FC_MAGIC_RUNES, FC_MAGIC_CHARGES, FC_MAGIC_TARGET,
    FC_MAGIC_BUSY, FC_MAGIC_QUEUE, FC_MAGIC_ALREADY_DRAINED
} FcMagicResult;

int fc_spell_count(void);
const FcSpellDef *fc_spell_at(int index);
const FcSpellDef *fc_spell_definition(int id);
const char *fc_spellbook_name(int book);
const char *fc_magic_result_message(FcMagicResult result);
FcMagicResult fc_set_spellbook(FcState *state, int book);
FcMagicResult fc_set_autocast(FcState *state, int spell_id);
FcMagicResult fc_cast_spell(FcState *state, int spell_id, int npc_slot);
FcMagicResult fc_magic_check(const FcState *state, int spell_id, int autocast);
int fc_magic_attack_range(const FcPlayer *player);
int fc_magic_active(const FcPlayer *player);
int fc_magic_max_hit(const FcPlayer *player, const FcNpc *target, int spell_id);
int fc_magic_attack_roll(const FcPlayer *player, const FcNpc *target, int spell_id);

#endif
