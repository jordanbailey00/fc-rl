#ifndef FC_NPC_H
#define FC_NPC_H

#include "fc_types.h"

/* NPC stat table entry (one per NPC_TYPE) */
typedef struct {
    int max_hp;
    int attack_style;       /* FcAttackStyle: primary style (ranged/magic for dual-mode) */
    int attack_speed;       /* ticks between attacks */
    int attack_range;       /* primary attack range (1 for melee-only, 14 for ranged/magic) */
    int max_hit;            /* primary style max damage (tenths) */
    int melee_max_hit;      /* melee max damage for dual-mode NPCs (0 if melee-only or no melee) */
    int att_level;
    int att_bonus;
    int def_level;          /* NPC defence level (for player attack accuracy) */
    int def_bonus;          /* NPC defence bonus vs ranged (simplified) */
    int magic_level;        /* used by Twisted bow scaling */
    int size;               /* tile footprint */
    int movement_speed;     /* 1=walk, 2=run */
    int prayer_drain;       /* base prayer drain in tenths (Tz-Kih specific) */
    int heal_amount;        /* HP healed per proc */
    int heal_interval;      /* ticks between independent Yt-HurKot heals */
    int jad_ranged_max_hit; /* Jad ranged max hit (0 for non-Jad) */
} FcNpcStats;

/* Get stats for a given NPC type */
const FcNpcStats* fc_npc_get_stats(int npc_type);

/* Initialize an NPC slot from type and spawn position */
void fc_npc_spawn(FcNpc* npc, int npc_type, int x, int y, int spawn_index);

/* True when the NPC could attack the player if its top-left footprint stood
 * at candidate_x,candidate_y. This checks attack style, range, melee contact,
 * and static LOS. */
int fc_npc_position_can_attack_player(const FcState* state, const FcNpc* npc,
                                      int candidate_x, int candidate_y);

/* Run NPC AI for one tick: movement + attack decision */
void fc_npc_tick(FcState* state, int npc_idx);

/* Tz-Kek split-on-death: spawn 2 NPC_TZ_KEK_SM at death position */
void fc_npc_tz_kek_split(FcState* state, int dead_x, int dead_y);

#endif /* FC_NPC_H */
