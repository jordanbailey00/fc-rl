#include "fc_npc.h"
#include "fc_combat.h"
#include "fc_pathfinding.h"
#include "fc_api.h"

/*
 * fc_npc.c — NPC framework with stat table and type-specific AI dispatch.
 *
 * PR 5: All 8 NPC types have full AI.
 *
 * NPC AI per tick (generic):
 *   1. If dead or inactive, skip.
 *   2. Decrement attack timer.
 *   3. Type-specific behavior (Jad style selection, Yt-MejKot heal, Yt-HurKot heal).
 *   4. If not in attack range, move toward player (greedy step).
 *   5. If in range and attack timer ready, roll attack and queue pending hit.
 *
 * Type-specific:
 *   Tz-Kih:     Melee + prayer drain on hit.
 *   Tz-Kek:     Melee. Splits into 2 small Tz-Kek on death.
 *   Tz-Kek-Sm:  Melee. (no special)
 *   Tok-Xil:    Ranged with projectile delay.
 *   Yt-MejKot:  Melee + heals nearby NPCs on timer.
 *   Ket-Zek:    Magic with projectile delay.
 *   TzTok-Jad:  Random magic/ranged attack selection, melee at range 1.
 *   Yt-HurKot:  Heals Jad unless distracted by player attack.
 */

/* ======================================================================== */
/* NPC stat table                                                            */
/* ======================================================================== */

/*
 * Stats sourced from OSRS wiki + Kotlin archive.
 * HP in tenths (100 = 10.0 HP). Max hit in tenths.
 * Defence stats used by fc_npc_def_roll for player attack accuracy.
 *
 * Fields: max_hp, attack_style, attack_speed, attack_range, max_hit,
 *         att_level, att_bonus, def_level, def_bonus, magic_level, size,
 *         movement_speed, prayer_drain, heal_amount, heal_interval, jad_ranged_max_hit
 */
/*
 * Stats from Void 634 cache + tzhaar_fight_cave.npcs.toml + tzhaar_fight_cave.combat.toml.
 * Sizes verified via NPCDecoder opcode 12 from Void 634 cache (2026-04-02 audit).
 *
 * Fields: max_hp, attack_style, attack_speed, attack_range, max_hit, melee_max_hit,
 *         att_level, att_bonus, def_level, def_bonus, magic_level, size, movement_speed,
 *         prayer_drain, heal_amount, heal_interval, jad_ranged_max_hit
 *
 * Dual-mode NPCs (Tok-Xil, Ket-Zek, Jad):
 *   attack_style = primary ranged/magic style, max_hit = primary style max
 *   melee_max_hit > 0 = can also melee at range 1
 */
static const FcNpcStats NPC_STATS[NPC_TYPE_COUNT] = {
    /* NPC_NONE */
    { 0, ATTACK_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

    /* NPC_TZ_KIH: Lv 22 melee bat. Drains 1 prayer on hit (10 tenths).
     * Void 634: HP 100, Att 20, Str 30, Def 15, size 1, stab max 40 */
    { 100, ATTACK_MELEE, 4, 1, 40, 0, 20, 0, 15, 0, 0, 1, 1, 10, 0, 0, 0 },

    /* NPC_TZ_KEK: Lv 45 melee blob. Splits into 2 small on death.
     * Void 634: HP 200, Att 40, Str 60, Def 30, size 2, crush max 70 */
    { 200, ATTACK_MELEE, 4, 1, 70, 0, 40, 0, 30, 0, 0, 2, 1, 0, 0, 0, 0 },

    /* NPC_TZ_KEK_SM: Lv 22 small blob (from split).
     * Void 634: HP 100, Att 20, Str 30, Def 15, size 1, crush max 40 */
    { 100, ATTACK_MELEE, 4, 1, 40, 0, 20, 0, 15, 0, 0, 1, 1, 0, 0, 0, 0 },

    /* NPC_TOK_XIL: Lv 90 ranged + melee (DUAL MODE).
     * Void 634: HP 400, Att 80, Str 120, Def 60, Rng 120, size 3
     * combat.toml: melee crush max 130 (range 1), ranged max 140 (range 14) */
    { 400, ATTACK_RANGED, 4, 14, 140, 130, 120, 0, 60, 0, 0, 3, 1, 0, 0, 0, 0 },

    /* NPC_YT_MEJKOT: Lv 180 melee + heals nearby NPCs with HP < 50% max.
     * Void 634: HP 800, Att 160, Str 240, Def 120, size 4
     * combat.toml: crush max 250. Heals 100 HP to nearby NPCs. */
    { 800, ATTACK_MELEE, 4, 1, 250, 0, 160, 0, 120, 0, 0, 4, 1, 0, 100, 4, 0 },

    /* NPC_KET_ZEK: Lv 360 magic + melee (DUAL MODE).
     * Void 634: HP 1600, Att 320, Str 480, Def 240, Mag 240, size 5
     * combat.toml: melee stab max 540 (range 1), magic max 490 (range 14) */
    { 1600, ATTACK_MAGIC, 4, 14, 490, 540, 240, 0, 240, 0, 240, 5, 1, 0, 0, 0, 0 },

    /* NPC_TZTOK_JAD: Lv 702 magic + ranged + melee.
     * Void 634: HP 2500, Att 640, Str 960, Def 480, Mag 480, Rng 960, size 5
     * combat.toml: melee stab max 970 (range 1), magic max 950 (range 14), ranged max 970
     * attack speed 8 (double normal), range 14 */
    { 2500, ATTACK_MAGIC, 8, 14, 970, 970, 480, 0, 480, 0, 480, 5, 1, 0, 0, 0, 950 },

    /* NPC_YT_HURKOT: Lv 108 Jad healer. Heals Jad 50 HP every 4 ticks within 5 tiles.
     * Void 634: HP 600, Att 140, Str 100, Def 60, size 1
     * combat.toml: crush max 140 */
    { 600, ATTACK_MELEE, 4, 1, 140, 0, 140, 0, 60, 0, 0, 1, 1, 0, 50, 4, 0 },
};

const FcNpcStats* fc_npc_get_stats(int npc_type) {
    if (npc_type < 0 || npc_type >= NPC_TYPE_COUNT) return &NPC_STATS[0];
    return &NPC_STATS[npc_type];
}

/* ======================================================================== */
/* Spawn                                                                     */
/* ======================================================================== */

void fc_npc_spawn(FcNpc* npc, int npc_type, int x, int y, int spawn_index) {
    const FcNpcStats* stats = fc_npc_get_stats(npc_type);

    npc->active = 1;
    npc->npc_type = npc_type;
    npc->spawn_index = spawn_index;
    npc->x = x;
    npc->y = y;
    npc->size = stats->size;
    npc->current_hp = stats->max_hp;
    npc->max_hp = stats->max_hp;
    npc->is_dead = 0;
    npc->attack_style = stats->attack_style;
    npc->attack_timer = stats->attack_speed;  /* first attack after full cooldown */
    npc->attack_speed = stats->attack_speed;
    npc->attack_range = stats->attack_range;
    npc->max_hit = stats->max_hit;
    npc->movement_speed = stats->movement_speed;
    npc->heal_timer = stats->heal_interval;  /* start at full cooldown */
    npc->heal_amount = stats->heal_amount;
    npc->healer_distracted = 0;
    npc->heal_target_idx = -1;
    npc->damage_taken_this_tick = 0;
    npc->died_this_tick = 0;
    npc->num_pending_hits = 0;
}

static void build_npc_movement_occupancy(
    const FcState* state,
    int npc_idx,
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]) {
    fc_build_occupancy(state, occupied, npc_idx, 0);
    if (state->movement_start_occupied_valid) {
        fc_overlay_occupancy(occupied, state->movement_start_occupied);
        if (npc_idx >= 0 && npc_idx < FC_MAX_NPCS &&
            state->movement_start_npc_active[npc_idx]) {
            fc_unmark_footprint_occupied(
                occupied,
                state->movement_start_npc_x[npc_idx],
                state->movement_start_npc_y[npc_idx],
                state->movement_start_npc_size[npc_idx]);
        }
    }
}

static int npc_dynamic_step_toward(FcState* state, int npc_idx,
                                   int target_x, int target_y) {
    FcNpc* npc = &state->npcs[npc_idx];
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];
    build_npc_movement_occupancy(state, npc_idx, occupied);
    return fc_npc_step_toward_sized_dynamic(&npc->x, &npc->y,
                                            target_x, target_y, npc->size,
                                            state->walkable, occupied);
}

static int min_i(int a, int b) {
    return a < b ? a : b;
}

static int max_i(int a, int b) {
    return a > b ? a : b;
}

static void npc_naive_player_chase_destination(const FcNpc* npc,
                                               const FcPlayer* player,
                                               int* out_x, int* out_y) {
    int source_width = npc->size;
    int source_length = npc->size;
    int target_width = 1;
    int target_length = 1;
    int diagonal = (npc->x - player->x) + (npc->y - player->y);
    int anti = (npc->x - player->x) - (npc->y - player->y);
    int south_west_clockwise = anti < 0;
    int north_west_clockwise =
        diagonal >= (target_length - 1) - (source_width - 1);
    int north_east_clockwise = anti > source_width - source_length;
    int south_east_clockwise =
        diagonal <= (target_width - 1) - (source_length - 1);

    if (south_west_clockwise && !north_west_clockwise) {
        int off_y;
        if (diagonal >= -source_width) {
            off_y = min_i(diagonal + source_width, target_length - 1);
        } else if (anti > -source_width) {
            off_y = -(source_width + anti);
        } else {
            off_y = 0;
        }
        *out_x = player->x - source_width;
        *out_y = player->y + off_y;
    } else if (north_west_clockwise && !north_east_clockwise) {
        int off_x;
        if (anti >= -target_length) {
            off_x = min_i(anti + target_length, target_width - 1);
        } else if (diagonal < target_length) {
            off_x = max_i(diagonal - target_length, -(source_width - 1));
        } else {
            off_x = 0;
        }
        *out_x = player->x + off_x;
        *out_y = player->y + target_length;
    } else if (north_east_clockwise && !south_east_clockwise) {
        int off_y;
        if (anti <= target_width) {
            off_y = target_length - anti;
        } else if (diagonal < target_width) {
            off_y = max_i(diagonal - target_width, -(source_length - 1));
        } else {
            off_y = 0;
        }
        *out_x = player->x + target_width;
        *out_y = player->y + off_y;
    } else {
        int off_x;
        if (diagonal > -source_length) {
            off_x = min_i(diagonal + source_length, target_width - 1);
        } else if (anti < source_length) {
            off_x = max_i(anti - source_length, -(source_length - 1));
        } else {
            off_x = 0;
        }
        *out_x = player->x + off_x;
        *out_y = player->y - source_length;
    }
}

int fc_npc_position_can_attack_player(const FcState* state, const FcNpc* npc,
                                      int candidate_x, int candidate_y) {
    if (!state || !npc || !npc->active || npc->is_dead) return 0;

    const FcNpcStats* stats = fc_npc_get_stats(npc->npc_type);
    const FcPlayer* p = &state->player;
    FcNpc candidate = *npc;
    candidate.x = candidate_x;
    candidate.y = candidate_y;

    int can_melee = fc_npc_can_melee_player(p->x, p->y,
                                            candidate.x, candidate.y,
                                            candidate.size, state->walkable);
    if (can_melee &&
        (stats->melee_max_hit > 0 || candidate.attack_style == ATTACK_MELEE)) {
        return 1;
    }

    if (candidate.attack_style != ATTACK_MELEE &&
        fc_distance_to_npc(p->x, p->y, &candidate) <= candidate.attack_range) {
        return fc_has_los_to_npc(p->x, p->y,
                                 candidate.x, candidate.y, candidate.size,
                                 state->walkable);
    }

    return 0;
}

static int npc_dynamic_step_toward_player_bounds(FcState* state, int npc_idx) {
    FcNpc* npc = &state->npcs[npc_idx];
    int target_x;
    int target_y;

    npc_naive_player_chase_destination(npc, &state->player,
                                       &target_x, &target_y);
    if (target_x == npc->x && target_y == npc->y) return 0;

    return npc_dynamic_step_toward(state, npc_idx, target_x, target_y);
}

static int split_spawn_tile_valid(const FcState* state, int x, int y, int size) {
    return fc_footprint_available_for_entity(state, x, y, size, -1, 0);
}

static int find_valid_split_spawn(const FcState* state,
                                  int preferred_x, int preferred_y, int size,
                                  int* out_x, int* out_y) {
    if (split_spawn_tile_valid(state, preferred_x, preferred_y, size)) {
        *out_x = preferred_x;
        *out_y = preferred_y;
        return 1;
    }

    for (int r = 1; r < FC_ARENA_WIDTH; r++) {
        for (int dx = -r; dx <= r; dx++) {
            for (int dy = -r; dy <= r; dy++) {
                if (dx != -r && dx != r && dy != -r && dy != r) continue;
                int nx = preferred_x + dx;
                int ny = preferred_y + dy;
                if (split_spawn_tile_valid(state, nx, ny, size)) {
                    *out_x = nx;
                    *out_y = ny;
                    return 1;
                }
            }
        }
    }

    return 0;
}

/* ======================================================================== */
/* Tz-Kek: split on death — spawn 2 small Tz-Kek                            */
/* ======================================================================== */

void fc_npc_tz_kek_split(FcState* state, int dead_x, int dead_y) {
    /* Spawn 2 NPC_TZ_KEK_SM at/near the death position.
     * Do NOT increment npcs_remaining — the parent Tz-Kek was pre-counted as 2
     * at wave spawn time. These children inherit that count and decrement normally
     * when they die. (Matches RSPS: parent not in npcDespawn list, children are.) */
    int spawned = 0;
    const FcNpcStats* child_stats = fc_npc_get_stats(NPC_TZ_KEK_SM);
    for (int i = 0; i < FC_MAX_NPCS && spawned < 2; i++) {
        if (!state->npcs[i].active) {
            int sx = dead_x + (spawned == 0 ? 0 : 1);
            int sy = dead_y;
            /* Clamp to arena */
            if (sx >= FC_ARENA_WIDTH - 1) sx = dead_x - 1;
            if (sx < 1) sx = 1;

            if (!find_valid_split_spawn(state, sx, sy, child_stats->size, &sx, &sy)) {
                continue;
            }

            fc_npc_spawn(&state->npcs[i], NPC_TZ_KEK_SM, sx, sy,
                         state->next_spawn_index++);
            /* No npcs_remaining++ — already pre-counted */
            spawned++;
        }
    }
}

/* ======================================================================== */
/* Jad direct attack selection                                               */
/* ======================================================================== */

static void jad_attack(FcState* state, FcNpc* npc, int npc_idx) {
    const FcNpcStats* stats = fc_npc_get_stats(npc->npc_type);
    FcPlayer* p = &state->player;
    int dist = fc_distance_to_npc(p->x, p->y, npc);
    int can_melee = fc_npc_can_melee_player(p->x, p->y, npc->x, npc->y,
                                            npc->size, state->walkable);

    if (npc->attack_timer > 0) return;

    int use_style = ATTACK_NONE;
    int use_max_hit = 0;
    int in_range = 0;

    if (can_melee && stats->melee_max_hit > 0) {
        use_style = ATTACK_MELEE;
        use_max_hit = stats->melee_max_hit;
        in_range = 1;
    } else if (dist <= npc->attack_range &&
               fc_has_los_to_npc(p->x, p->y, npc->x, npc->y, npc->size,
                                 state->walkable)) {
        use_style = (fc_rng_int(state, 2) == 0) ? ATTACK_MAGIC : ATTACK_RANGED;
        use_max_hit = (use_style == ATTACK_MAGIC) ? stats->max_hit : stats->jad_ranged_max_hit;
        in_range = 1;
    }

    if (!in_range) return;

    int att_roll = fc_npc_attack_roll(stats->att_level, stats->att_bonus);
    int def_roll = fc_player_def_roll(p, use_style);
    float chance = fc_hit_chance(att_roll, def_roll);

    int hit = (fc_rng_float(state) < chance) ? 1 : 0;
    int damage = hit ? fc_rng_int(state, use_max_hit + 1) : 0;
    int delay = fc_npc_hit_delay(npc->npc_type, use_style, dist);

    if (fc_queue_pending_hit(p->pending_hits, &p->num_pending_hits,
                             FC_MAX_PENDING_HITS,
                             damage, delay, use_style, npc_idx, 0)) {
        FcPendingHit* queued = &p->pending_hits[p->num_pending_hits - 1];
        if (use_style == ATTACK_MELEE) {
            queued->prayer_snapshot = p->prayer;
        } else {
            queued->prayer_snapshot = -1;
            queued->prayer_lock_tick = state->tick + 1;
        }
    }

    npc->attack_timer = npc->attack_speed;
}

/* ======================================================================== */
/* Yt-MejKot: heal nearby NPCs                                              */
/* ======================================================================== */

static void record_npc_heal(FcState* state, int amount, int source_type, int target_type) {
    if (amount <= 0) return;
    state->npc_heal_procs_this_tick++;
    state->npc_heal_amount_this_tick += amount;
    if (source_type == NPC_YT_MEJKOT) {
        state->mejkot_heal_amount_this_tick += amount;
    }
    if (target_type == NPC_TZTOK_JAD) {
        state->jad_heal_amount_this_tick += amount;
    }
}

static void yt_mejkot_heal_tick(FcState* state, FcNpc* npc) {
    if (!fc_npc_can_melee_player(state->player.x, state->player.y,
                                 npc->x, npc->y, npc->size, state->walkable)) {
        return;
    }

    if (npc->heal_timer > 0) {
        npc->heal_timer--;
        return;
    }

    const FcNpcStats* stats = fc_npc_get_stats(npc->npc_type);
    npc->heal_timer = stats->heal_interval;

    /* Heal nearby NPCs (within 8 tiles) that have HP < 50% of max.
     * Condition from RSPS: "weakened_nearby_monsters" = Constitution < max/2.
     * Heals first qualifying NPC found, not all. */
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* target = &state->npcs[i];
        if (!target->active || target->is_dead) continue;
        if (target == npc) continue;  /* don't self-heal */
        if (target->current_hp >= target->max_hp / 2) continue;  /* HP must be < 50% */

        /* Chebyshev distance between NPCs */
        int dx = (npc->x > target->x) ? npc->x - target->x : target->x - npc->x;
        int dy = (npc->y > target->y) ? npc->y - target->y : target->y - npc->y;
        int dist = (dx > dy) ? dx : dy;

        if (dist <= 8) {
            int before = target->current_hp;
            target->current_hp += npc->heal_amount;
            if (target->current_hp > target->max_hp) {
                target->current_hp = target->max_hp;
            }
            record_npc_heal(state, target->current_hp - before,
                            npc->npc_type, target->npc_type);
            return;  /* single heal per attack cycle */
        }
    }
}

/* ======================================================================== */
/* Yt-HurKot: heal Jad unless distracted                                     */
/* ======================================================================== */

static void yt_hurkot_tick(FcState* state, FcNpc* npc, int npc_idx) {
    if (npc->healer_distracted) return;  /* player attacked us, stop healing */

    if (npc->heal_timer > 0) {
        npc->heal_timer--;
        return;
    }

    const FcNpcStats* stats = fc_npc_get_stats(npc->npc_type);
    npc->heal_timer = stats->heal_interval;

    /* Find Jad and heal him */
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* jad = &state->npcs[i];
        if (jad->npc_type == NPC_TZTOK_JAD && jad->active && !jad->is_dead) {
            npc->heal_target_idx = i;

            /* Move toward Jad if not adjacent */
            int dx = (npc->x > jad->x) ? npc->x - jad->x : jad->x - npc->x;
            int dy = (npc->y > jad->y) ? npc->y - jad->y : jad->y - npc->y;
            int dist = (dx > dy) ? dx : dy;

            if (dist <= 5) {
                /* Within heal range — heal Jad */
                if (jad->current_hp < jad->max_hp) {
                    int before = jad->current_hp;
                    jad->current_hp += npc->heal_amount;
                    if (jad->current_hp > jad->max_hp) {
                        jad->current_hp = jad->max_hp;
                    }
                    if (jad->current_hp > before) {
                        state->jad_heal_procs_this_tick++;
                        record_npc_heal(state, jad->current_hp - before,
                                        npc->npc_type, jad->npc_type);
                    }
                }
            } else {
                /* Walk toward Jad instead of player (size-aware) */
                npc_dynamic_step_toward(state, npc_idx, jad->x, jad->y);
            }
            return;
        }
    }
}

/* ======================================================================== */
/* Generic NPC attack (melee/ranged/magic, non-Jad)                          */
/* ======================================================================== */

/*
 * Dual-mode NPCs (Tok-Xil, Ket-Zek):
 *   - If in melee range (dist <= 1) AND melee_max_hit > 0: use melee
 *   - Otherwise if in primary range AND has LOS: use primary (ranged/magic)
 *   - This matches RSPS combat.toml where both attacks are valid but
 *     distance determines which is selected.
 */
static void npc_generic_attack(FcState* state, FcNpc* npc, int npc_idx) {
    const FcNpcStats* stats = fc_npc_get_stats(npc->npc_type);
    FcPlayer* p = &state->player;
    int dist = fc_distance_to_npc(p->x, p->y, npc);
    int can_melee = fc_npc_can_melee_player(p->x, p->y, npc->x, npc->y,
                                            npc->size, state->walkable);

    if (npc->attack_timer > 0) return;

    /* Determine attack style and max hit based on distance */
    int use_style = npc->attack_style;  /* primary style */
    int use_max_hit = npc->max_hit;
    int in_range = 0;

    if (can_melee && stats->melee_max_hit > 0) {
        /* In melee range and has melee capability → melee */
        use_style = ATTACK_MELEE;
        use_max_hit = stats->melee_max_hit;
        in_range = 1;
    } else if (can_melee && npc->attack_style == ATTACK_MELEE) {
        /* Pure melee NPC, in range */
        in_range = 1;
    } else if (dist <= npc->attack_range && npc->attack_style != ATTACK_MELEE) {
        /* Primary ranged/magic — need LOS */
        if (fc_has_los_to_npc(p->x, p->y, npc->x, npc->y, npc->size,
                              state->walkable)) {
            in_range = 1;
        }
        /* No LOS → don't attack, NPC will continue chasing */
    }

    if (!in_range) return;

    int att_roll = fc_npc_attack_roll(stats->att_level, stats->att_bonus);
    int def_roll = fc_player_def_roll(p, use_style);
    float chance = fc_hit_chance(att_roll, def_roll);

    int hit = (fc_rng_float(state) < chance) ? 1 : 0;
    int damage = hit ? fc_rng_int(state, use_max_hit + 1) : 0;

    int delay = fc_npc_hit_delay(npc->npc_type, use_style, dist);

    if (fc_queue_pending_hit(p->pending_hits, &p->num_pending_hits,
                             FC_MAX_PENDING_HITS,
                             damage, delay, use_style, npc_idx,
                             stats->prayer_drain)) {
        FcPendingHit* queued = &p->pending_hits[p->num_pending_hits - 1];
        queued->prayer_snapshot = p->prayer;
    }

    npc->attack_timer = npc->attack_speed;
}

static int npc_has_attack_position(FcState* state, FcNpc* npc) {
    return fc_npc_position_can_attack_player(state, npc, npc->x, npc->y);
}

/* ======================================================================== */
/* NPC AI tick — type dispatch                                               */
/* ======================================================================== */

void fc_npc_tick(FcState* state, int npc_idx) {
    FcNpc* npc = &state->npcs[npc_idx];
    if (!npc->active || npc->is_dead) return;

    /* Decrement attack timer */
    if (npc->attack_timer > 0) npc->attack_timer--;

    /* --- Type-specific pre-attack behavior --- */

    /* Yt-HurKot: heal Jad instead of normal AI (unless distracted) */
    if (npc->npc_type == NPC_YT_HURKOT && !npc->healer_distracted) {
        yt_hurkot_tick(state, npc, npc_idx);
        return;  /* healers don't attack when healing Jad */
    }

    /* Yt-MejKot: heal nearby NPCs on timer */
    if (npc->npc_type == NPC_YT_MEJKOT) {
        yt_mejkot_heal_tick(state, npc);
    }

    /* Jad: move into range, then choose its attack style when the hit is queued */
    if (npc->npc_type == NPC_TZTOK_JAD) {
        if (!npc_has_attack_position(state, npc)) {
            for (int step = 0; step < npc->movement_speed; step++) {
                if (!npc_dynamic_step_toward_player_bounds(state, npc_idx)) break;
            }
        }
        jad_attack(state, npc, npc_idx);
        return;
    }

    /* --- Generic movement + attack for all other types --- */

    /* Movement: keep walking until this tile can actually attack. */
    if (!npc_has_attack_position(state, npc)) {
        for (int step = 0; step < npc->movement_speed; step++) {
            if (!npc_dynamic_step_toward_player_bounds(state, npc_idx)) break;
        }
    }

    /* Attack */
    npc_generic_attack(state, npc, npc_idx);
}
