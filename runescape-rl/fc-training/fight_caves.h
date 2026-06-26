/*
 * fight_caves.h — PufferLib 4.0 environment wrapper for Fight Caves.
 *
 * Wraps the FC backend (fc_*.c) into PufferLib's c_reset/c_step/c_render
 * interface. All game logic lives in the fc_*.c files included below.
 * This file only handles the PufferLib adapter layer:
 *   - FightCaves struct with PufferLib-required fields
 *   - c_reset: init game state, compute initial obs
 *   - c_step: read actions, step game, compute reward+obs, handle terminal
 *   - c_render: launch Raylib viewer (eval mode only)
 *   - c_close: cleanup
 *
 * Single-agent environment (num_agents=1 always for Fight Caves).
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Include all backend sources directly (compiled as one unit) */
#include "fc_types.h"
#include "fc_contracts.h"
#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_pathfinding.h"
#include "fc_prayer.h"
#include "fc_reward.h"
#include "fc_wave.h"

/* Backend .c files — included directly so fight_caves.h is self-contained.
 * PufferLib compiles binding.c which includes this header. */
#include "fc_rng.c"
#include "fc_pathfinding.c"
#include "fc_prayer.c"
#include "fc_combat.c"
#include "fc_npc.c"
#include "fc_wave.c"
#include "fc_tick.c"
#include "fc_state.c"

/* Raylib rendering for eval mode (Phase 11).
 * Only compiled when FC_RENDER is defined (build.sh --render). */
#ifdef FC_RENDER
#include "fc_render.h"
#endif

/* ======================================================================== */
/* PufferLib Log struct (required fields)                                    */
/* ======================================================================== */

typedef struct {
    float episode_length;
    float wave_reached;
    float npcs_slayed;
    float prayer_uptime_melee;
    float prayer_uptime_range;
    float prayer_uptime_magic;
    float correct_prayer;
    float wrong_prayer_hits;
    float no_prayer_hits;
    float prayer_switches;
    float damage_blocked;
    float dmg_taken_avg;
    float attack_when_ready_rate;
    float invalid_move;
    float invalid_attack;
    float invalid_prayer;
    float pots_used;
    float avg_prayer_on_pot;
    float food_eaten;
    float avg_hp_on_food;
    float food_wasted;
    float pots_wasted;
    float tokxil_melee_ticks;
    float ketzek_melee_ticks;
    float max_wave_ticks;
    float max_wave_ticks_wave;
    float reached_wave_63;
    float jad_kill_rate;
    float rwd_sum[FC_CH_COUNT];
    float rwd_fires[FC_CH_COUNT];
    float n;  /* must be last */
} Log;

/* ======================================================================== */
/* PufferLib Environment struct                                              */
/* ======================================================================== */

/* Puffer-facing observation size:
 *   policy obs + masks for heads 0-2 only (move/attack/prayer)
 *   = 122 + 31 = 153
 */
#define FC_PUFFER_OBS_SIZE (FC_POLICY_OBS_SIZE + FC_MOVE_DIM + FC_ATTACK_DIM + FC_PRAYER_DIM)

/* Number of action heads for PufferLib: move, attack, prayer. */
#define FC_PUFFER_NUM_ATNS 3

typedef struct FightCaves {
    Log log;                    /* required by PufferLib */
    float* observations;        /* required: FC_PUFFER_OBS_SIZE per agent */
    float* actions;             /* required: NUM_ATNS per agent (vecenv uses float*) */
    float* rewards;             /* required: 1 per agent */
    float* terminals;           /* required: 1 per agent (vecenv uses float*) */
    int num_agents;             /* always 1 for Fight Caves */
    int rng;                    /* per-env RNG seed (set by vecenv.h) */

    /* Game state */
    FcState state;

    /* Reward shaping weights (from config) */
    float w_damage_dealt;       /* applied as (damage + damaging_hits) * w per tick */
    float w_damage_taken;
    float w_npc_kill;
    float w_wave_clear;
    float w_jad_kill;
    float w_player_death;
    float w_correct_jad_prayer;      /* fires only on Jad hits */
    float w_correct_danger_prayer;   /* fires on non-Jad styled hits (melee/ranged/magic) */
    float w_invalid_action;
    float w_tick_penalty;

    /* Configurable shaping terms (kept separate from reward-feature weights) */
    float shape_unnecessary_prayer_penalty;
    float shape_wave_stall_base_penalty;
    float shape_wave_stall_cap;
    float shape_jad_heal_penalty;
    int shape_wave_stall_start;
    int shape_wave_stall_ramp_interval;
    int initial_sharks;
    int initial_prayer_doses;
    int ticks_since_attack;      /* ticks since agent last dealt damage */
    int ticks_in_wave;           /* ticks since current wave started (reset on wave_clear) */

    /* Obs ablation flags (experimental — see fc_apply_obs_ablation in fc_state.c).
     * When non-zero, the corresponding obs slots are zeroed AFTER fc_write_obs.
     * Used by the OBS Sweep / Ablation experiment to test which features the
     * policy actually relies on vs. which the GRU could re-derive from the rest. */
    int obs_ablate_npc_distance;
    int obs_ablate_incoming_aggregates;
    int obs_ablate_npc_valid;

    int ep_length;

    /* Per-episode reward-channel analytics. Reset at c_reset, transferred to
     * the per-env PufferLib Log on terminal. See FcRwdChannel enum in
     * fc_reward.h for channel indices and names. */
    float ep_rwd_sum[FC_CH_COUNT];
    int   ep_rwd_fires[FC_CH_COUNT];

    /* RNG seed counter (increments each episode for variety) */
    uint32_t seed_counter;
} FightCaves;

/* ======================================================================== */
/* Observation writer — policy obs + action mask into flat float buffer      */
/* ======================================================================== */

static void fc_puffer_write_obs(FightCaves* env) {
    float* obs = env->observations;

    /* Policy observations */
    fc_write_obs(&env->state, obs);

    /* Optional obs ablation (zero specific feature slots in-place) */
    fc_apply_obs_ablation(obs,
                          env->obs_ablate_npc_distance,
                          env->obs_ablate_incoming_aggregates,
                          env->obs_ablate_npc_valid);

    /* Action mask: 31 floats (move/attack/prayer only). */
    float full_mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(&env->state, full_mask);
    int mask_size = FC_MOVE_DIM + FC_ATTACK_DIM + FC_PRAYER_DIM;
    memcpy(obs + FC_POLICY_OBS_SIZE, full_mask, sizeof(float) * mask_size);
}

static FcRewardParams fc_reward_params_from_env(const FightCaves* env) {
    FcRewardParams params;
    memset(&params, 0, sizeof(params));

    params.w_damage_dealt = env->w_damage_dealt;
    params.w_damage_taken = env->w_damage_taken;
    params.w_npc_kill = env->w_npc_kill;
    params.w_wave_clear = env->w_wave_clear;
    params.w_jad_kill = env->w_jad_kill;
    params.w_player_death = env->w_player_death;
    params.w_correct_jad_prayer = env->w_correct_jad_prayer;
    params.w_correct_danger_prayer = env->w_correct_danger_prayer;
    params.w_invalid_action = env->w_invalid_action;
    params.w_tick_penalty = env->w_tick_penalty;

    params.shape_unnecessary_prayer_penalty = env->shape_unnecessary_prayer_penalty;
    params.shape_wave_stall_base_penalty = env->shape_wave_stall_base_penalty;
    params.shape_wave_stall_cap = env->shape_wave_stall_cap;
    params.shape_jad_heal_penalty = env->shape_jad_heal_penalty;

    params.shape_wave_stall_start = env->shape_wave_stall_start;
    params.shape_wave_stall_ramp_interval = env->shape_wave_stall_ramp_interval;

    return params;
}

/* ======================================================================== */
/* Reward computation from reward features                                   */
/* ======================================================================== */

static float fc_puffer_compute_reward(FightCaves* env) {
    FcRewardParams params = fc_reward_params_from_env(env);
    FcRewardRuntime runtime = { env->ticks_since_attack, env->ticks_in_wave };
    FcRewardBreakdown breakdown =
        fc_reward_compute_breakdown(&env->state, &params, &runtime);

    env->ticks_since_attack = runtime.ticks_since_attack;
    env->ticks_in_wave = runtime.ticks_in_wave;

    if (breakdown.threat_ctx.tokxil_melee) env->state.ep_tokxil_melee_ticks++;
    if (breakdown.threat_ctx.ketzek_melee) env->state.ep_ketzek_melee_ticks++;

    /* Per-channel analytics: accumulate value and fire count per reward channel.
     * Drains into the per-env PufferLib Log on terminal; see c_step. */
    float ch[FC_CH_COUNT];
    fc_reward_breakdown_channels(&breakdown, ch);
    for (int i = 0; i < FC_CH_COUNT; i++) {
        env->ep_rwd_sum[i] += ch[i];
        if (ch[i] != 0.0f) env->ep_rwd_fires[i]++;
    }

    return breakdown.total;
}

/* ======================================================================== */
/* PufferLib interface: c_reset, c_step, c_render, c_close                   */
/* ======================================================================== */

static uint32_t fc_puffer_mix_reset_seed(uint32_t env_rng, uint32_t episode) {
    uint32_t x = env_rng + 0x9E3779B9u * (episode + 1u);
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return (x != 0u) ? x : 0x12345678u;
}

void c_reset(FightCaves* env) {
    env->seed_counter++;
    fc_reset(&env->state,
             fc_puffer_mix_reset_seed((uint32_t)env->rng, env->seed_counter));
    if (env->initial_sharks < 0) env->initial_sharks = 0;
    if (env->initial_sharks > FC_MAX_SHARKS) env->initial_sharks = FC_MAX_SHARKS;
    if (env->initial_prayer_doses < 0) env->initial_prayer_doses = 0;
    if (env->initial_prayer_doses > FC_MAX_PRAYER_DOSES)
        env->initial_prayer_doses = FC_MAX_PRAYER_DOSES;
    env->state.player.sharks_remaining = env->initial_sharks;
    env->state.player.prayer_doses_remaining = env->initial_prayer_doses;

    env->ep_length = 0;
    env->ticks_since_attack = 0;
    env->ticks_in_wave = 0;
    for (int i = 0; i < FC_CH_COUNT; i++) {
        env->ep_rwd_sum[i] = 0.0f;
        env->ep_rwd_fires[i] = 0;
    }

    /* Compute initial observations */
    fc_puffer_write_obs(env);
}

void c_step(FightCaves* env) {
    env->rewards[0] = 0.0f;
    env->terminals[0] = 0.0f;

    /* Convert float actions from network to int action heads.
     * PufferLib sends actions as floats in a flat array.
     * Puffer-facing no-supplies policy uses only move/attack/prayer.
     * Core heads 3-6 are left as zero: no eat, no drink, no walk-to-tile. */
    int actions[FC_NUM_ACTION_HEADS];
    memset(actions, 0, sizeof(actions));
    for (int h = 0; h < FC_PUFFER_NUM_ATNS; h++) {
        actions[h] = (int)env->actions[h];
    }
    /* Heads 5+6 (walk-to-tile) always 0 — not used in v1 */

    /* Step the game simulation */
    fc_step(&env->state, actions);

    /* Compute reward */
    float reward = fc_puffer_compute_reward(env);
    env->rewards[0] = reward;
    env->ep_length++;

    /* Write observations BEFORE checking terminal — agent must see
     * the terminal state observation, not the post-reset observation. */
    fc_puffer_write_obs(env);

    /* Check terminal */
    if (fc_is_terminal(&env->state)) {
        env->terminals[0] = 1.0f;
        env->log.episode_length += (float)env->ep_length;
        env->log.wave_reached += (float)env->state.current_wave;
        env->log.npcs_slayed += (float)env->state.total_npcs_killed;
        env->log.prayer_uptime_melee += (env->ep_length > 0)
            ? (float)env->state.ep_ticks_pray_melee / (float)env->ep_length : 0.0f;
        env->log.prayer_uptime_range += (env->ep_length > 0)
            ? (float)env->state.ep_ticks_pray_range / (float)env->ep_length : 0.0f;
        env->log.prayer_uptime_magic += (env->ep_length > 0)
            ? (float)env->state.ep_ticks_pray_magic / (float)env->ep_length : 0.0f;
        env->log.correct_prayer += (float)env->state.ep_correct_blocks;
        env->log.wrong_prayer_hits += (float)env->state.ep_wrong_prayer_hits;
        env->log.no_prayer_hits += (float)env->state.ep_no_prayer_hits;
        env->log.prayer_switches += (float)env->state.ep_prayer_switches;
        env->log.damage_blocked += (float)env->state.ep_damage_blocked;
        env->log.dmg_taken_avg += (float)env->state.player.total_damage_taken;
        env->log.attack_when_ready_rate += (env->state.ep_attack_ready_ticks > 0)
            ? (float)env->state.ep_attack_attempt_ticks / (float)env->state.ep_attack_ready_ticks
            : 0.0f;
        env->log.invalid_move +=
            (float)env->state.ep_invalid_action_classes[FC_INVALID_ACTION_MOVE];
        env->log.invalid_attack +=
            (float)env->state.ep_invalid_action_classes[FC_INVALID_ACTION_ATTACK];
        env->log.invalid_prayer +=
            (float)env->state.ep_invalid_action_classes[FC_INVALID_ACTION_PRAYER];
        env->log.pots_used += (float)env->state.ep_pots_used;
        env->log.avg_prayer_on_pot += (env->state.ep_pots_used > 0 && env->state.player.max_prayer > 0)
            ? ((float)env->state.ep_pot_pre_prayer_sum /
               ((float)env->state.ep_pots_used * (float)env->state.player.max_prayer)) : 0.0f;
        env->log.food_eaten += (float)env->state.ep_food_eaten;
        env->log.avg_hp_on_food += (env->state.ep_food_eaten > 0 && env->state.player.max_hp > 0)
            ? ((float)env->state.ep_food_pre_hp_sum /
               ((float)env->state.ep_food_eaten * (float)env->state.player.max_hp)) : 0.0f;
        env->log.food_wasted += (float)env->state.ep_food_overhealed;
        env->log.pots_wasted += (float)env->state.ep_pots_overrestored;
        env->log.tokxil_melee_ticks += (float)env->state.ep_tokxil_melee_ticks;
        env->log.ketzek_melee_ticks += (float)env->state.ep_ketzek_melee_ticks;
        env->log.max_wave_ticks += (float)env->state.ep_max_wave_ticks;
        env->log.max_wave_ticks_wave += (float)env->state.ep_max_wave_ticks_wave;
        env->log.reached_wave_63 += (float)env->state.ep_reached_wave_63;
        env->log.jad_kill_rate += (float)env->state.ep_jad_killed;

        for (int i = 0; i < FC_CH_COUNT; i++) {
            env->log.rwd_sum[i] += env->ep_rwd_sum[i];
            env->log.rwd_fires[i] += (float)env->ep_rwd_fires[i];
        }
        env->log.n += 1.0f;

        /* Auto-reset for next episode. Obs already written above
         * reflecting the terminal state. Next c_step will see the
         * reset state's obs after stepping. */
        c_reset(env);
    }
}

void c_render(FightCaves* env) {
    /* Rendering handled by external viewer via --policy-pipe mode.
     * See fc-viewer/eval_viewer.py for the eval pipeline. */
    (void)env;
}

void c_close(FightCaves* env) {
    fc_destroy(&env->state);
}
