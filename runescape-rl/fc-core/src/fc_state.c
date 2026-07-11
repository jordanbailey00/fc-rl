#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_wave.h"
#include "fc_pathfinding.h"
#include "fc_player_init.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * fc_state.c — State allocation, initialization, reset, rendering.
 *
 * FcState is caller-allocated (stack or heap). These functions
 * initialize and reset it. memset to zero is the canonical reset
 * mechanism — all fields must have safe zero defaults.
 */

/* ======================================================================== */
/* Arena collision map (from Void 634 cache, region 37,79, level 0)          */
/* ======================================================================== */

/*
 * Binary collision extracted via DumpFcCollision.kt from the Void 634 cache.
 * Format: 64*64 bytes, row-major (y=0..63, x=0..63), 1=walkable, 0=blocked.
 * 2153 walkable tiles, 1943 blocked. The irregular cave shape with walls,
 * pillars, and narrow passages is critical for safespot mechanics.
 *
 * Loaded from fc-core/assets/fightcaves.collision at runtime.
 * If the file is missing, falls back to an open arena with border walls.
 */
/* Cached collision data — loaded once, shared by all envs.
 * Avoids per-reset file I/O which is not thread-safe under OpenMP. */
static uint8_t g_collision_cache[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];
static int g_collision_loaded = 0;

static void load_collision_once(void) {
    if (g_collision_loaded) return;

    static const char* collision_paths[] = {
        "assets/fightcaves.collision",
        "runescape-rl/fc-core/assets/fightcaves.collision",
        "fc-core/assets/fightcaves.collision",
        "../fc-core/assets/fightcaves.collision",
        "../../fc-core/assets/fightcaves.collision",
        NULL
    };
    FILE* f = NULL;
    const char* env_path = getenv("FC_COLLISION_PATH");
    if (env_path) f = fopen(env_path, "rb");
    for (int p = 0; !f && collision_paths[p]; p++) {
        f = fopen(collision_paths[p], "rb");
    }
    if (f) {
        uint8_t buf[FC_ARENA_WIDTH * FC_ARENA_HEIGHT];
        size_t n = fread(buf, 1, sizeof(buf), f);
        fclose(f);
        if (n == sizeof(buf)) {
            /* Binary is row-major [y][x], our array is [x][y] — transpose */
            for (int y = 0; y < FC_ARENA_HEIGHT; y++) {
                for (int x = 0; x < FC_ARENA_WIDTH; x++) {
                    g_collision_cache[x][y] = buf[y * FC_ARENA_WIDTH + x];
                }
            }
            g_collision_loaded = 1;
            return;
        }
    }

    /* Fallback: open arena with border walls */
    fprintf(stderr,
            "warning: fightcaves.collision not found; using open fallback arena\n");
    memset(g_collision_cache, 1, sizeof(g_collision_cache));
    for (int x = 0; x < FC_ARENA_WIDTH; x++) {
        g_collision_cache[x][0] = 0;
        g_collision_cache[x][FC_ARENA_HEIGHT - 1] = 0;
    }
    for (int y = 0; y < FC_ARENA_HEIGHT; y++) {
        g_collision_cache[0][y] = 0;
        g_collision_cache[FC_ARENA_WIDTH - 1][y] = 0;
    }
    g_collision_loaded = 1;
}

static void setup_arena(FcState* state) {
    load_collision_once();
    memcpy(state->walkable, g_collision_cache, sizeof(state->walkable));
}

/* Player initialization — constants defined in fc_player_init.h */

static void init_player(FcPlayer* p) {
    p->x = FC_ARENA_WIDTH / 2;
    p->y = FC_ARENA_HEIGHT / 2;
    p->current_hp = FC_PLAYER_MAX_HP;
    p->max_hp = FC_PLAYER_MAX_HP;
    p->current_prayer = FC_PLAYER_MAX_PRAYER;
    p->max_prayer = FC_PLAYER_MAX_PRAYER;
    p->prayer = PRAYER_NONE;
    p->sharks_remaining = FC_MAX_SHARKS;
    p->prayer_doses_remaining = FC_MAX_PRAYER_DOSES;
    p->attack_timer = 0;
    p->food_timer = 0;
    p->potion_timer = 0;
    p->combo_timer = 0;
    p->run_energy = 10000;
    p->is_running = 1;
    p->attack_level = FC_PLAYER_ATTACK_LVL;
    p->strength_level = FC_PLAYER_STRENGTH_LVL;
    p->defence_level = FC_PLAYER_DEFENCE_LVL;
    p->ranged_level = FC_PLAYER_RANGED_LVL;
    p->prayer_level = FC_PLAYER_PRAYER_LVL;
    p->magic_level = FC_PLAYER_MAGIC_LVL;
    p->weapon_kind = FC_PLAYER_WEAPON_KIND;
    p->weapon_uses_ammo = FC_PLAYER_WEAPON_USES_AMMO;
    p->weapon_speed = FC_PLAYER_WEAPON_SPEED;
    p->weapon_range = FC_PLAYER_WEAPON_RANGE;
    p->ranged_attack_bonus = FC_EQUIP_RANGED_ATK;
    p->ranged_strength_bonus = FC_EQUIP_RANGED_STR;
    p->defence_stab = FC_EQUIP_DEF_STAB;
    p->defence_slash = FC_EQUIP_DEF_SLASH;
    p->defence_crush = FC_EQUIP_DEF_CRUSH;
    p->defence_magic = FC_EQUIP_DEF_MAGIC;
    p->defence_ranged = FC_EQUIP_DEF_RANGED;
    p->prayer_bonus = FC_EQUIP_PRAYER_BONUS;
    p->ammo_count = FC_PLAYER_AMMO;
    p->hp_regen_counter = 0;
    p->route_len = 0;
    p->route_idx = 0;
    p->attack_target_idx = -1;
    p->approach_target = 0;
}

/* ======================================================================== */
/* Lifecycle                                                                 */
/* ======================================================================== */

void fc_init(FcState* state) {
    memset(state, 0, sizeof(FcState));
}

void fc_reset(FcState* state, uint32_t seed) {
    /* Zero everything first — ensures no stale state, padding is clean */
    memset(state, 0, sizeof(FcState));

    /* Seed RNG */
    fc_rng_seed(state, seed);

    /* Select random rotation */
    state->rotation_id = fc_rng_int(state, FC_NUM_ROTATIONS);

    /* Setup arena */
    setup_arena(state);

    /* Initialize player */
    init_player(&state->player);

    /* Spawn wave 1 NPCs */
    state->current_wave = 1;
    state->next_spawn_index = 0;
    state->wave_start_tick = 0;
    fc_wave_spawn(state, 1);
}

void fc_step(FcState* state, const int actions[FC_NUM_ACTION_HEADS]) {
    if (state->terminal != TERMINAL_NONE) return;  /* episode over */
    fc_tick(state, actions);
}

void fc_destroy(FcState* state) {
    /* Currently no heap allocations. Zero for safety. */
    memset(state, 0, sizeof(FcState));
}

/* ======================================================================== */
/* Observation / Mask / Reward                                               */
/* ======================================================================== */

/* Sort helper: indices of active NPCs, sorted by (distance, spawn_index) */
static int compare_npc_slots(const void* a, const void* b, const FcState* state) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    int da = fc_distance_to_npc(state->player.x, state->player.y, &state->npcs[ia]);
    int db = fc_distance_to_npc(state->player.x, state->player.y, &state->npcs[ib]);
    if (da != db) return da - db;
    return state->npcs[ia].spawn_index - state->npcs[ib].spawn_index;
}

/* Simple insertion sort for small arrays (max 16 elements) */
static void sort_npc_indices(int* indices, int count, const FcState* state) {
    for (int i = 1; i < count; i++) {
        int key = indices[i];
        int j = i - 1;
        while (j >= 0 && compare_npc_slots(&key, &indices[j], state) < 0) {
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = key;
    }
}

int fc_visible_npc_indices(const FcState* state, int out_indices[FC_VISIBLE_NPCS]) {
    int active_indices[FC_MAX_NPCS];
    int active_count = 0;

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        if (state->npcs[i].active && !state->npcs[i].is_dead) {
            active_indices[active_count++] = i;
        }
    }

    sort_npc_indices(active_indices, active_count, state);

    int visible = (active_count < FC_VISIBLE_NPCS) ? active_count : FC_VISIBLE_NPCS;
    for (int slot = 0; slot < visible; slot++) {
        out_indices[slot] = active_indices[slot];
    }
    return visible;
}

static int move_action_valid_with_occupancy(
    const FcState* state,
    int action,
    const uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]);

static int move_action_valid(const FcState* state, int action) {
    const FcPlayer* p = &state->player;
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];

    if (action < 0 || action >= FC_MOVE_DIM) return 0;
    if (action == FC_MOVE_IDLE) return 1;
    if (action >= FC_MOVE_RUN_N && p->run_energy <= 0) {
        return 0;
    }

    fc_build_occupancy(state, occupied, -1, 1);
    return move_action_valid_with_occupancy(state, action, occupied);
}

static int move_action_valid_with_occupancy(
    const FcState* state,
    int action,
    const uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT]) {
    const FcPlayer* p = &state->player;

    if (action < 0 || action >= FC_MOVE_DIM) return 0;
    if (action == FC_MOVE_IDLE) return 1;
    if (action >= FC_MOVE_RUN_N && p->run_energy <= 0) {
        return 0;
    }

    int tx = p->x;
    int ty = p->y;
    int max_steps = (action >= FC_MOVE_RUN_N) ? 2 : 1;
    return fc_move_toward_dynamic(&tx, &ty, FC_MOVE_DX[action], FC_MOVE_DY[action],
                                  max_steps, state->walkable, occupied) > 0;
}

static int attack_action_valid(const FcState* state, int action) {
    int visible_indices[FC_VISIBLE_NPCS];
    int visible;
    int slot;

    if (action < 0 || action >= FC_ATTACK_DIM) return 0;
    if (action == FC_ATTACK_NONE) return 1;

    visible = fc_visible_npc_indices(state, visible_indices);
    slot = action - 1;
    return slot >= 0 && slot < visible;
}

static int prayer_action_valid(int action) {
    return action >= 0 && action < FC_PRAYER_DIM;
}

static int eat_action_valid(const FcState* state, int action) {
    const FcPlayer* p = &state->player;

    if (action < 0 || action >= FC_EAT_DIM) return 0;
    if (action == FC_EAT_NONE) return 1;
    if (action == FC_EAT_SHARK) {
        return p->sharks_remaining > 0 &&
               p->food_timer <= 0 &&
               p->current_hp < p->max_hp;
    }
    if (action == FC_EAT_COMBO) {
        return p->sharks_remaining > 0 &&
               p->combo_timer <= 0 &&
               p->current_hp < p->max_hp;
    }
    return 0;
}

static int drink_action_valid(const FcState* state, int action) {
    const FcPlayer* p = &state->player;

    if (action < 0 || action >= FC_DRINK_DIM) return 0;
    if (action == FC_DRINK_NONE) return 1;
    if (action == FC_DRINK_PRAYER_POT) {
        return p->prayer_doses_remaining > 0 &&
               p->potion_timer <= 0 &&
               p->current_prayer < p->max_prayer;
    }
    return 0;
}

static int attack_style_summary_idx(int style) {
    switch (style) {
        case ATTACK_MELEE:  return 0;
        case ATTACK_RANGED: return 1;
        case ATTACK_MAGIC:  return 2;
        default:            return -1;
    }
}

static float normalize_incoming_count(int count) {
    if (count <= 0) return 0.0f;
    if (count >= 4) return 1.0f;
    return (float)count / 4.0f;
}

static float clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static float normalize_prayer_drain_counter(const FcPlayer* p) {
    int resistance = 60 + 2 * p->prayer_bonus;
    if (resistance <= 0) return 0.0f;
    float normalized = (float)p->prayer_drain_counter / (float)resistance;
    if (normalized < 0.0f) return 0.0f;
    if (normalized > 1.0f) return 1.0f;
    return normalized;
}

static int pending_hit_prayer_actionable(const FcPendingHit* ph) {
    if (!ph->active) return 0;
    if (ph->prayer_snapshot >= 0) return 0;
    if (ph->prayer_lock_tick < 0) return 0;
    return attack_style_summary_idx(ph->attack_style) >= 0;
}

static float pending_hit_prayer_deadline_urgency(const FcState* state,
                                                 const FcPendingHit* ph) {
    if (!pending_hit_prayer_actionable(ph)) return 0.0f;

    int ticks_until_lock = ph->prayer_lock_tick - state->tick;
    if (ticks_until_lock < 0) ticks_until_lock = 0;
    if (ticks_until_lock > 3) ticks_until_lock = 3;

    return (float)(4 - ticks_until_lock) / 4.0f;
}

/* Distance-only attack-style telegraph: what style would this NPC throw if it
 * attacked right now from its current position? Does NOT check LOS; the LOS
 * bit is a separate obs feature so the agent can distinguish "melee threat,
 * safespotted" (style=MELEE, LOS=0) from "melee threat, can hit me" (style=MELEE,
 * LOS=1). Returns ATTACK_NONE for empty slots, dead NPCs, Yt-HurKot (healer,
 * don't want policy praying for it), and Jad before it has committed a
 * pending_hit (style is stochastic until commit). */
static int npc_telegraph_style(const FcState* state, const FcNpc* npc) {
    if (!npc->active || npc->is_dead) return ATTACK_NONE;
    if (npc->npc_type == NPC_YT_HURKOT) return ATTACK_NONE;

    if (npc->npc_type == NPC_TZTOK_JAD) {
        /* Jad alternates magic/ranged randomly at commit; read the committed
         * style from the queued pending_hit. No prediction before commit. */
        const FcPlayer* p = &state->player;
        for (int i = 0; i < p->num_pending_hits; i++) {
            const FcPendingHit* ph = &p->pending_hits[i];
            if (ph->active && ph->source_npc_idx >= 0 &&
                state->npcs[ph->source_npc_idx].npc_type == NPC_TZTOK_JAD) {
                return ph->attack_style;
            }
        }
        return ATTACK_NONE;
    }

    const FcNpcStats* stats = fc_npc_get_stats(npc->npc_type);
    int can_melee = fc_npc_can_melee_player(state->player.x, state->player.y,
                                            npc->x, npc->y, npc->size,
                                            state->walkable);
    /* Dual-mode NPCs (Tok-Xil, Ket-Zek): melee when adjacent, primary otherwise.
     * Pure melee NPCs (Yt-MejKot, Tz-Kih, Tz-Kek): telegraph MELEE even when far
     * — they'll close the gap and that's what they'll hit with. */
    if (can_melee && (stats->melee_max_hit > 0 || npc->attack_style == ATTACK_MELEE)) {
        return ATTACK_MELEE;
    }
    return npc->attack_style;
}

static int npc_type_obs_offset(int npc_type) {
    switch (npc_type) {
        case NPC_TZ_KIH: return FC_NPC_TYPE_TZ_KIH;
        case NPC_TZ_KEK: return FC_NPC_TYPE_TZ_KEK;
        case NPC_TZ_KEK_SM: return FC_NPC_TYPE_TZ_KEK_SM;
        case NPC_TOK_XIL: return FC_NPC_TYPE_TOK_XIL;
        case NPC_YT_MEJKOT: return FC_NPC_TYPE_YT_MEJKOT;
        case NPC_KET_ZEK: return FC_NPC_TYPE_KET_ZEK;
        case NPC_TZTOK_JAD: return FC_NPC_TYPE_TZTOK_JAD;
        case NPC_YT_HURKOT: return FC_NPC_TYPE_YT_HURKOT;
        default: return -1;
    }
}

void fc_write_obs(const FcState* state, float* out) {
    memset(out, 0, sizeof(float) * FC_TOTAL_OBS);

    const FcPlayer* p = &state->player;
    int incoming_counts[3][3] = {{0}};
    float prayer_deadline_urgency[3] = {0.0f, 0.0f, 0.0f};

    /* Compact incoming-hit timeline summary.
     * Counts by style for hits landing in 1, 2, and 3 ticks. This gives the
     * policy a relative timing signal without leaking absolute episode clocks.
     * Prayer deadline urgency is separate: it marks pending hits whose prayer
     * snapshot has not locked yet, which is the actual decision window. */
    for (int hi = 0; hi < p->num_pending_hits; hi++) {
        const FcPendingHit* ph = &p->pending_hits[hi];
        if (!ph->active) continue;
        int style_idx = attack_style_summary_idx(ph->attack_style);
        if (style_idx >= 0 && pending_hit_prayer_actionable(ph)) {
            float urgency = pending_hit_prayer_deadline_urgency(state, ph);
            if (urgency > prayer_deadline_urgency[style_idx]) {
                prayer_deadline_urgency[style_idx] = urgency;
            }
        }
        if (ph->ticks_remaining < 1 || ph->ticks_remaining > 3) continue;
        if (style_idx < 0) continue;
        int bucket = ph->ticks_remaining - 1;
        if (incoming_counts[bucket][style_idx] < 4) {
            incoming_counts[bucket][style_idx]++;
        }
    }

    /* Player features */
    float* player = out + FC_OBS_PLAYER_START;
    player[FC_OBS_PLAYER_HP]        = (p->max_hp > 0) ? (float)p->current_hp / (float)p->max_hp : 0.0f;
    player[FC_OBS_PLAYER_PRAYER]    = (p->max_prayer > 0) ? (float)p->current_prayer / (float)p->max_prayer : 0.0f;
    player[FC_OBS_PLAYER_X]         = (float)p->x / (float)FC_ARENA_WIDTH;
    player[FC_OBS_PLAYER_Y]         = (float)p->y / (float)FC_ARENA_HEIGHT;
    player[FC_OBS_PLAYER_ATK_TIMER] = (p->weapon_speed > 0)
        ? (float)p->attack_timer / (float)p->weapon_speed : 0.0f;
    player[FC_OBS_PLAYER_PRAY_MEL]  = (p->prayer == PRAYER_PROTECT_MELEE) ? 1.0f : 0.0f;
    player[FC_OBS_PLAYER_PRAY_RNG]  = (p->prayer == PRAYER_PROTECT_RANGE) ? 1.0f : 0.0f;
    player[FC_OBS_PLAYER_PRAY_MAG]  = (p->prayer == PRAYER_PROTECT_MAGIC) ? 1.0f : 0.0f;
    player[FC_OBS_PLAYER_SHARKS]    = (float)p->sharks_remaining / (float)FC_MAX_SHARKS;
    player[FC_OBS_PLAYER_DOSES]     = (float)p->prayer_doses_remaining / (float)FC_MAX_PRAYER_DOSES;
    player[FC_OBS_PLAYER_IN_MEL_1T] = normalize_incoming_count(incoming_counts[0][0]);
    player[FC_OBS_PLAYER_IN_RNG_1T] = normalize_incoming_count(incoming_counts[0][1]);
    player[FC_OBS_PLAYER_IN_MAG_1T] = normalize_incoming_count(incoming_counts[0][2]);
    player[FC_OBS_PLAYER_IN_MEL_2T] = normalize_incoming_count(incoming_counts[1][0]);
    player[FC_OBS_PLAYER_IN_RNG_2T] = normalize_incoming_count(incoming_counts[1][1]);
    player[FC_OBS_PLAYER_IN_MAG_2T] = normalize_incoming_count(incoming_counts[1][2]);
    player[FC_OBS_PLAYER_TARGET]    = 0.0f;  /* filled after NPC slot computation below */
    player[FC_OBS_PLAYER_PRAY_DDL_MEL] = prayer_deadline_urgency[0];
    player[FC_OBS_PLAYER_PRAY_DDL_RNG] = prayer_deadline_urgency[1];
    player[FC_OBS_PLAYER_PRAY_DDL_MAG] = prayer_deadline_urgency[2];

    /* NPC slot selection: gather active NPCs, sort, take first 8 */
    int active_indices[FC_VISIBLE_NPCS];
    int visible = fc_visible_npc_indices(state, active_indices);
    for (int slot = 0; slot < visible; slot++) {
        const FcNpc* n = &state->npcs[active_indices[slot]];
        float* npc_out = out + FC_OBS_NPC_START + slot * FC_OBS_NPC_STRIDE;

        npc_out[FC_NPC_VALID]         = 1.0f;
        npc_out[FC_NPC_X]             = (float)n->x / (float)FC_ARENA_WIDTH;
        npc_out[FC_NPC_Y]             = (float)n->y / (float)FC_ARENA_HEIGHT;
        npc_out[FC_NPC_HP]            = (n->max_hp > 0) ? (float)n->current_hp / (float)n->max_hp : 0.0f;
        npc_out[FC_NPC_DISTANCE]      =
            (float)fc_distance_to_npc(p->x, p->y, n) / (float)FC_ARENA_WIDTH;
        float has_los = (float)fc_has_los_to_npc(
            p->x, p->y, n->x, n->y, n->size, state->walkable);
        int tele = npc_telegraph_style(state, n);
        npc_out[FC_NPC_TELE_MELEE]    = (tele == ATTACK_MELEE)  ? 1.0f : 0.0f;
        npc_out[FC_NPC_TELE_RANGED]   = (tele == ATTACK_RANGED) ? 1.0f : 0.0f;
        npc_out[FC_NPC_TELE_MAGIC]    = (tele == ATTACK_MAGIC)  ? 1.0f : 0.0f;
        npc_out[FC_NPC_ATK_TIMER]     = (n->attack_speed > 0) ? (float)n->attack_timer / (float)n->attack_speed : 0.0f;
        npc_out[FC_NPC_LOS]           = has_los;
        int type_offset = npc_type_obs_offset(n->npc_type);
        if (type_offset >= 0) {
            npc_out[type_offset] = 1.0f;
        }

        /* Pending attack from this NPC — scan player's pending hits */
        npc_out[FC_NPC_PENDING_STYLE] = 0.0f;
        npc_out[FC_NPC_PENDING_TICKS] = 0.0f;
        npc_out[FC_NPC_PENDING_PRAYER_WINDOW] = 0.0f;
        npc_out[FC_NPC_PENDING_PRAYER_DEADLINE] = 0.0f;
        for (int hi = 0; hi < p->num_pending_hits; hi++) {
            const FcPendingHit* ph = &p->pending_hits[hi];
            if (ph->active && ph->source_npc_idx == active_indices[slot]) {
                npc_out[FC_NPC_PENDING_STYLE] = (float)ph->attack_style / 3.0f;
                npc_out[FC_NPC_PENDING_TICKS] = (float)ph->ticks_remaining / 10.0f;
                if (pending_hit_prayer_actionable(ph)) {
                    npc_out[FC_NPC_PENDING_PRAYER_WINDOW] = 1.0f;
                    npc_out[FC_NPC_PENDING_PRAYER_DEADLINE] =
                        pending_hit_prayer_deadline_urgency(state, ph);
                }
                break;  /* report first pending hit from this NPC */
            }
        }
    }
    /* Remaining NPC slots already zeroed by memset */

    /* Player target: which visible NPC slot is the current attack target */
    if (p->attack_target_idx >= 0) {
        for (int s = 0; s < visible; s++) {
            if (active_indices[s] == p->attack_target_idx) {
                player[FC_OBS_PLAYER_TARGET] = (float)(s + 1) / 8.0f;
                break;
            }
        }
    }

    /* Wave/meta features */
    float* meta = out + FC_OBS_META_START;
    meta[FC_OBS_META_WAVE]       = (float)state->current_wave / (float)FC_NUM_WAVES;
    meta[FC_OBS_META_ROTATION]   = (float)state->rotation_id / (float)FC_NUM_ROTATIONS;
    meta[FC_OBS_META_REMAINING]  = (float)state->npcs_remaining / (float)FC_MAX_NPCS;
    meta[FC_OBS_META_PRAY_DRAIN] = normalize_prayer_drain_counter(p);
    meta[FC_OBS_META_IN_MEL_3T]  = normalize_incoming_count(incoming_counts[2][0]);
    meta[FC_OBS_META_IN_RNG_3T]  = normalize_incoming_count(incoming_counts[2][1]);
    meta[FC_OBS_META_IN_MAG_3T]  = normalize_incoming_count(incoming_counts[2][2]);
    meta[FC_OBS_META_DMG_T_TICK] = (p->max_hp > 0) ? (float)state->damage_taken_this_tick / (float)p->max_hp : 0.0f;
    meta[FC_OBS_META_WAVE_CLR]   = (float)state->wave_just_cleared;
    meta[FC_OBS_META_CAVE_PROG]  = clamp01(state->progress_cave_progress);
    meta[FC_OBS_META_WAVE_PROG]  = clamp01(state->progress_current_wave_progress);
    meta[FC_OBS_META_WORK_REM]   = (state->progress_required_work_start > 0.0f)
        ? clamp01(state->progress_required_work_remaining /
                  state->progress_required_work_start)
        : 0.0f;
    meta[FC_OBS_META_NO_PROG]    = clamp01((float)state->progress_ticks_since_positive / 2400.0f);

    /* Reward features (at offset FC_REWARD_START) — written by fc_write_reward_features */
    fc_write_reward_features(state, out + FC_REWARD_START);
}

void fc_apply_obs_ablation(float* out,
                           int ablate_npc_distance,
                           int ablate_incoming_aggregates,
                           int ablate_npc_valid) {
    if (ablate_npc_distance) {
        for (int s = 0; s < FC_OBS_NPC_SLOTS; s++) {
            out[FC_OBS_NPC_START + s * FC_OBS_NPC_STRIDE + FC_NPC_DISTANCE] = 0.0f;
        }
    }
    if (ablate_incoming_aggregates) {
        out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MEL_1T] = 0.0f;
        out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_RNG_1T] = 0.0f;
        out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MAG_1T] = 0.0f;
        out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MEL_2T] = 0.0f;
        out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_RNG_2T] = 0.0f;
        out[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MAG_2T] = 0.0f;
        out[FC_OBS_META_START + FC_OBS_META_IN_MEL_3T] = 0.0f;
        out[FC_OBS_META_START + FC_OBS_META_IN_RNG_3T] = 0.0f;
        out[FC_OBS_META_START + FC_OBS_META_IN_MAG_3T] = 0.0f;
    }
    if (ablate_npc_valid) {
        for (int s = 0; s < FC_OBS_NPC_SLOTS; s++) {
            out[FC_OBS_NPC_START + s * FC_OBS_NPC_STRIDE + FC_NPC_VALID] = 0.0f;
        }
    }
}

void fc_write_reward_features(const FcState* state, float* out) {
    memset(out, 0, sizeof(float) * FC_REWARD_FEATURES);

    out[FC_RWD_DAMAGE_DEALT]     = (float)state->damage_dealt_this_tick / 1000.0f;
    out[FC_RWD_DAMAGE_TAKEN]     = (state->player.max_hp > 0) ?
                                   (float)state->damage_taken_this_tick / (float)state->player.max_hp : 0.0f;
    out[FC_RWD_NPC_KILL]         = (float)state->npcs_killed_this_tick;
    out[FC_RWD_WAVE_CLEAR]       = (float)state->wave_just_cleared;
    out[FC_RWD_JAD_DAMAGE]       = (float)state->jad_damage_this_tick / 1000.0f;
    out[FC_RWD_JAD_KILL]         = (float)state->jad_killed;
    out[FC_RWD_PLAYER_DEATH]     = (state->terminal == TERMINAL_PLAYER_DEATH) ? 1.0f : 0.0f;
    out[FC_RWD_CAVE_COMPLETE]    = (state->terminal == TERMINAL_CAVE_COMPLETE) ? 1.0f : 0.0f;
    out[FC_RWD_FOOD_USED]        = (float)state->food_used_this_tick;
    out[FC_RWD_PRAYER_POT_USED]  = (float)state->prayer_potion_used_this_tick;
    out[FC_RWD_CORRECT_JAD_PRAY] = (float)state->correct_jad_prayer;
    out[FC_RWD_WRONG_JAD_PRAY]   = (float)state->wrong_jad_prayer;
    out[FC_RWD_INVALID_ACTION]   = (float)state->invalid_action_this_tick;
    out[FC_RWD_MOVEMENT]         = (float)state->movement_this_tick;
    out[FC_RWD_IDLE]             = (float)state->idle_this_tick;
    out[FC_RWD_TICK_PENALTY]     = 1.0f;  /* always fires */
    out[FC_RWD_CORRECT_DANGER_PRAY] = (float)state->correct_danger_prayer;
    out[FC_RWD_WRONG_DANGER_PRAY]   = (float)state->wrong_danger_prayer;
    out[FC_RWD_ATTACK_ATTEMPT]      = (float)state->attack_attempt_this_tick;
    out[FC_RWD_PRAYER_LOST]         = (float)state->prayer_lost_this_tick / 10.0f;
}

void fc_action_invalid_classes(const FcState* state,
                               const int actions[FC_NUM_ACTION_HEADS],
                               int out_classes[FC_INVALID_ACTION_CLASS_COUNT]) {
    /* Keep this aligned with the Puffer-facing policy mask surface:
     * move, attack, and prayer only. Consumable and path-target heads remain
     * canonical core actions, but are not emitted by the no-supplies policy. */
    out_classes[FC_INVALID_ACTION_MOVE] = !move_action_valid(state, actions[0]);
    out_classes[FC_INVALID_ACTION_ATTACK] = !attack_action_valid(state, actions[1]);
    out_classes[FC_INVALID_ACTION_PRAYER] = !prayer_action_valid(actions[2]);
}

int fc_action_attempt_is_invalid(const FcState* state, const int actions[FC_NUM_ACTION_HEADS]) {
    int invalid_classes[FC_INVALID_ACTION_CLASS_COUNT];

    fc_action_invalid_classes(state, actions, invalid_classes);
    for (int i = 0; i < FC_INVALID_ACTION_CLASS_COUNT; i++) {
        if (invalid_classes[i]) return 1;
    }
    return 0;
}

void fc_write_mask(const FcState* state, float* out) {
    /* Set all to valid, then mask invalid */
    for (int i = 0; i < FC_ACTION_MASK_SIZE; i++) {
        out[i] = 1.0f;
    }

    /* MOVE: idle always valid. Walk/run directions masked if destination not walkable */
    uint8_t movement_occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];
    fc_build_occupancy(state, movement_occupied, -1, 1);
    for (int m = 1; m < FC_MOVE_DIM; m++) {
        if (!move_action_valid_with_occupancy(state, m, movement_occupied)) {
            out[FC_MASK_MOVE_START + m] = 0.0f;
        }
    }

    /* ATTACK: slot 0 (none) always valid. Slots 1-8 masked if no NPC in that slot */
    for (int attack = FC_ATTACK_NONE + 1; attack < FC_ATTACK_DIM; attack++) {
        if (!attack_action_valid(state, attack)) {
            out[FC_MASK_ATTACK_START + attack] = 0.0f;
        }
    }

    /* PRAYER: leave fully unmasked.
     * Prayer toggles may be legal even when they are redundant or no-op, and
     * the policy should learn those costs from the environment rather than
     * having them hidden by the mask. */

    /* EAT */
    if (!eat_action_valid(state, FC_EAT_SHARK)) {
        out[FC_MASK_EAT_START + FC_EAT_SHARK] = 0.0f;
    }
    if (!eat_action_valid(state, FC_EAT_COMBO)) {
        out[FC_MASK_EAT_START + FC_EAT_COMBO] = 0.0f;
    }

    /* DRINK */
    if (!drink_action_valid(state, FC_DRINK_PRAYER_POT)) {
        out[FC_MASK_DRINK_START + FC_DRINK_PRAYER_POT] = 0.0f;
    }
}

int fc_is_terminal(const FcState* state) {
    return state->terminal != TERMINAL_NONE;
}

int fc_terminal_code(const FcState* state) {
    return state->terminal;
}

/* ======================================================================== */
/* Render entities                                                           */
/* ======================================================================== */

void fc_fill_render_entities(const FcState* state, FcRenderEntity* entities, int* count) {
    int idx = 0;

    /* Entity 0: player */
    FcRenderEntity* pe = &entities[idx++];
    memset(pe, 0, sizeof(FcRenderEntity));
    pe->entity_type = ENTITY_PLAYER;
    pe->x = state->player.x;
    pe->y = state->player.y;
    pe->size = 1;
    pe->current_hp = state->player.current_hp;
    pe->max_hp = state->player.max_hp;
    pe->prayer = state->player.prayer;
    pe->damage_taken_this_tick = state->player.damage_taken_this_tick;
    pe->hit_landed_this_tick = state->player.hit_landed_this_tick;

    /* Active NPCs + NPCs that just died this tick (for death hitsplat visibility) */
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        const FcNpc* n = &state->npcs[i];
        if (!n->active && !n->died_this_tick) continue;

        FcRenderEntity* ne = &entities[idx++];
        memset(ne, 0, sizeof(FcRenderEntity));
        ne->entity_type = ENTITY_NPC;
        ne->npc_type = n->npc_type;
        ne->x = n->x;
        ne->y = n->y;
        ne->size = n->size;
        ne->current_hp = n->current_hp;
        ne->max_hp = n->max_hp;
        ne->attack_style = n->attack_style;
        ne->is_dead = n->is_dead;
        ne->damage_taken_this_tick = n->damage_taken_this_tick;
        ne->died_this_tick = n->died_this_tick;
        ne->npc_slot = i;
    }

    *count = idx;
}
