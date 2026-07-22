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
    float tokxil_melee_ticks;
    float ketzek_melee_ticks;
    float max_wave_ticks;
    float max_wave_ticks_wave;
    float reached_wave_63;
    float jad_kill_rate;
    float dmg_to_npc_type[NPC_TYPE_COUNT];
    float resolved_hits_to_npc_type[NPC_TYPE_COUNT];
    float damaging_hits_to_npc_type[NPC_TYPE_COUNT];
    float attack_cycles_to_npc_type[NPC_TYPE_COUNT];
    float target_ticks_by_npc_type[NPC_TYPE_COUNT];
    float target_held_ticks;
    float no_target_ticks;
    float target_in_range_los_ticks;
    float target_out_of_range_or_los_ticks;
    float attack_cooldown_wait_ticks;
    float ready_but_no_attack_ticks;
    float action_move_idle_ticks;
    float action_move_walk_ticks;
    float action_move_run_ticks;
    float action_attack_none_ticks;
    float action_attack_target_ticks;
    float action_prayer_noop_ticks;
    float action_prayer_cmd_ticks;
    float no_progress_ticks;
    float no_progress_idle_move_ticks;
    float no_progress_move_cmd_ticks;
    float no_progress_attack_none_ticks;
    float no_progress_attack_target_ticks;
    float no_progress_has_target_ticks;
    float no_progress_no_target_ticks;
    float no_progress_prayer_cmd_ticks;
    float no_progress_invalid_action_ticks;
    float required_work_remaining;
    float required_work_start;
    float cave_progress;
    float current_wave_progress;
    float progress_delta;
    float progress_reward;
    float ticks_since_positive_progress;
    float positive_progress_ticks;
    float zero_progress_ticks;
    float negative_progress_ticks;
    float gross_damage_dealt;
    float net_required_work_removed;
    float gross_damage_to_net_progress_ratio;
    float npc_healing_total;
    float mejkot_healing_total;
    float jad_healing_total;
    float preclip_reward;
    float postclip_reward;
    float positive_clip_count;
    float negative_clip_count;
    float rwd_sum[FC_CH_COUNT];
    float rwd_fires[FC_CH_COUNT];
    float n;  /* must be last */
} Log;

/* ======================================================================== */
/* PufferLib Environment struct                                              */
/* ======================================================================== */

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
    float w_damage_dealt;       /* legacy per-hit damage shaping; active fc_revamp config sets 0 */
    float w_progress;
    float w_damage_taken;
    float w_npc_kill;
    float w_wave_clear;
    float w_jad_kill;
    float w_cave_complete;
    float w_player_death;
    float w_correct_jad_prayer;      /* optional additional Jad-only bonus */
    float w_correct_danger_prayer;   /* shared correct-block reward, including Jad */
    float w_prayer_lost;             /* per prayer point lost from overhead drain or Tz-Kih */
    float w_invalid_action;
    float w_tick_penalty;

    /* Configurable shaping terms (kept separate from reward-feature weights) */
    float shape_unnecessary_prayer_penalty;
    float shape_wave_stall_base_penalty;
    float shape_wave_stall_cap;
    float shape_jad_heal_penalty;
    float shape_npc_heal_penalty;
    float shape_no_progress_penalty_1;
    float shape_no_progress_penalty_2;
    float shape_no_progress_penalty_3;
    float shape_no_attack_base_penalty;
    float shape_no_attack_wave_scale;
    int shape_wave_stall_start;
    int shape_wave_stall_ramp_interval;
    int shape_no_progress_start_1;
    int shape_no_progress_start_2;
    int shape_no_progress_start_3;
    int shape_no_attack_start;
    int initial_sharks;
    int initial_prayer_doses;
    FcRewardRuntime reward_runtime;

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

    /* Puffer-action no-progress diagnostics. These are adapter-level metrics
     * for stall-like ticks: no movement, no attack cycle, no damage, no kill,
     * no wave clear, and not just normal in-range weapon cooldown waiting. */
    float ep_no_progress_ticks;
    float ep_no_progress_idle_move_ticks;
    float ep_no_progress_move_cmd_ticks;
    float ep_no_progress_attack_none_ticks;
    float ep_no_progress_attack_target_ticks;
    float ep_no_progress_has_target_ticks;
    float ep_no_progress_no_target_ticks;
    float ep_no_progress_prayer_cmd_ticks;
    float ep_no_progress_invalid_action_ticks;
    float ep_progress_delta;
    float ep_progress_reward;
    float ep_gross_damage_dealt;
    float ep_net_required_work_removed;
    float ep_npc_healing_total;
    float ep_mejkot_healing_total;
    float ep_jad_healing_total;
    float ep_preclip_reward;
    float ep_postclip_reward;
    float ep_positive_clip_count;
    float ep_negative_clip_count;

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

    /* Action mask: Puffer policy heads only. */
    float full_mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(&env->state, full_mask);
    memcpy(obs + FC_POLICY_OBS_SIZE, full_mask, sizeof(float) * FC_PUFFER_MASK_SIZE);
}

static FcRewardParams fc_reward_params_from_env(const FightCaves* env) {
    FcRewardParams params;
    memset(&params, 0, sizeof(params));

    params.w_damage_dealt = env->w_damage_dealt;
    params.w_progress = env->w_progress;
    params.w_damage_taken = env->w_damage_taken;
    params.w_npc_kill = env->w_npc_kill;
    params.w_wave_clear = env->w_wave_clear;
    params.w_jad_kill = env->w_jad_kill;
    params.w_cave_complete = env->w_cave_complete;
    params.w_player_death = env->w_player_death;
    params.w_correct_jad_prayer = env->w_correct_jad_prayer;
    params.w_correct_danger_prayer = env->w_correct_danger_prayer;
    params.w_prayer_lost = env->w_prayer_lost;
    params.w_invalid_action = env->w_invalid_action;
    params.w_tick_penalty = env->w_tick_penalty;

    params.shape_unnecessary_prayer_penalty = env->shape_unnecessary_prayer_penalty;
    params.shape_wave_stall_base_penalty = env->shape_wave_stall_base_penalty;
    params.shape_wave_stall_cap = env->shape_wave_stall_cap;
    params.shape_jad_heal_penalty = env->shape_jad_heal_penalty;
    params.shape_npc_heal_penalty = env->shape_npc_heal_penalty;
    params.shape_no_progress_penalty_1 = env->shape_no_progress_penalty_1;
    params.shape_no_progress_penalty_2 = env->shape_no_progress_penalty_2;
    params.shape_no_progress_penalty_3 = env->shape_no_progress_penalty_3;
    params.shape_no_attack_base_penalty = env->shape_no_attack_base_penalty;
    params.shape_no_attack_wave_scale = env->shape_no_attack_wave_scale;

    params.shape_wave_stall_start = env->shape_wave_stall_start;
    params.shape_wave_stall_ramp_interval = env->shape_wave_stall_ramp_interval;
    params.shape_no_progress_start_1 = env->shape_no_progress_start_1;
    params.shape_no_progress_start_2 = env->shape_no_progress_start_2;
    params.shape_no_progress_start_3 = env->shape_no_progress_start_3;
    params.shape_no_attack_start = env->shape_no_attack_start;

    return params;
}

/* ======================================================================== */
/* Reward computation from reward features                                   */
/* ======================================================================== */

static float fc_puffer_compute_reward(FightCaves* env) {
    FcRewardParams params = fc_reward_params_from_env(env);
    FcRewardBreakdown breakdown =
        fc_reward_compute_breakdown(&env->state, &params, &env->reward_runtime);
    fc_reward_sync_progress_state(&env->state, &env->reward_runtime);

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

    env->ep_progress_delta += env->reward_runtime.last_progress_delta;
    env->ep_progress_reward += breakdown.progress;
    env->ep_gross_damage_dealt += (float)env->state.damage_dealt_this_tick;
    env->ep_net_required_work_removed += env->reward_runtime.last_net_required_work_removed;
    env->ep_npc_healing_total += (float)env->state.npc_heal_amount_this_tick;
    env->ep_mejkot_healing_total += (float)env->state.mejkot_heal_amount_this_tick;
    env->ep_jad_healing_total += (float)env->state.jad_heal_amount_this_tick;
    env->ep_preclip_reward += breakdown.total;
    {
        float clipped = breakdown.total;
        if (clipped > 1.0f) {
            clipped = 1.0f;
            env->ep_positive_clip_count += 1.0f;
        } else if (clipped < -1.0f) {
            clipped = -1.0f;
            env->ep_negative_clip_count += 1.0f;
        }
        env->ep_postclip_reward += clipped;
    }

    return breakdown.total;
}

static void fc_puffer_reset_episode_action_diagnostics(FightCaves* env) {
    env->ep_no_progress_ticks = 0.0f;
    env->ep_no_progress_idle_move_ticks = 0.0f;
    env->ep_no_progress_move_cmd_ticks = 0.0f;
    env->ep_no_progress_attack_none_ticks = 0.0f;
    env->ep_no_progress_attack_target_ticks = 0.0f;
    env->ep_no_progress_has_target_ticks = 0.0f;
    env->ep_no_progress_no_target_ticks = 0.0f;
    env->ep_no_progress_prayer_cmd_ticks = 0.0f;
    env->ep_no_progress_invalid_action_ticks = 0.0f;
    env->ep_progress_delta = 0.0f;
    env->ep_progress_reward = 0.0f;
    env->ep_gross_damage_dealt = 0.0f;
    env->ep_net_required_work_removed = 0.0f;
    env->ep_npc_healing_total = 0.0f;
    env->ep_mejkot_healing_total = 0.0f;
    env->ep_jad_healing_total = 0.0f;
    env->ep_preclip_reward = 0.0f;
    env->ep_postclip_reward = 0.0f;
    env->ep_positive_clip_count = 0.0f;
    env->ep_negative_clip_count = 0.0f;
}

static void fc_puffer_record_no_progress_diagnostics(
    FightCaves* env,
    const int actions[FC_NUM_ACTION_HEADS]) {
    const FcState* state = &env->state;
    const FcPlayer* player = &state->player;
    int target_active = 0;

    if (state->terminal != TERMINAL_NONE || state->npcs_remaining <= 0) return;
    if (state->movement_this_tick || state->attack_attempt_this_tick) return;
    if (state->damage_dealt_this_tick > 0 || state->npcs_killed_this_tick > 0) return;
    if (state->wave_just_cleared) return;

    if (player->attack_target_idx >= 0) {
        const FcNpc* target = &state->npcs[player->attack_target_idx];
        target_active = target->active && !target->is_dead;
        if (target_active && player->attack_timer > 0) {
            int dist = fc_distance_to_npc(player->x, player->y, target);
            int has_los = fc_has_los_to_npc(
                player->x, player->y, target->x, target->y, target->size,
                state->walkable);
            if (dist <= player->weapon_range && has_los) return;
        }
    }

    env->ep_no_progress_ticks += 1.0f;
    if (actions[0] == FC_MOVE_IDLE) {
        env->ep_no_progress_idle_move_ticks += 1.0f;
    } else {
        env->ep_no_progress_move_cmd_ticks += 1.0f;
    }
    if (actions[1] == FC_ATTACK_NONE) {
        env->ep_no_progress_attack_none_ticks += 1.0f;
    } else {
        env->ep_no_progress_attack_target_ticks += 1.0f;
    }
    if (target_active) {
        env->ep_no_progress_has_target_ticks += 1.0f;
    } else {
        env->ep_no_progress_no_target_ticks += 1.0f;
    }
    if (actions[2] != 0) {
        env->ep_no_progress_prayer_cmd_ticks += 1.0f;
    }
    if (state->invalid_action_this_tick) {
        env->ep_no_progress_invalid_action_ticks += 1.0f;
    }
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
    fc_reward_runtime_begin_episode(&env->reward_runtime, &env->state);
    for (int i = 0; i < FC_CH_COUNT; i++) {
        env->ep_rwd_sum[i] = 0.0f;
        env->ep_rwd_fires[i] = 0;
    }
    fc_puffer_reset_episode_action_diagnostics(env);

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
    fc_puffer_record_no_progress_diagnostics(env, actions);

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
        env->log.tokxil_melee_ticks += (float)env->state.ep_tokxil_melee_ticks;
        env->log.ketzek_melee_ticks += (float)env->state.ep_ketzek_melee_ticks;
        env->log.max_wave_ticks += (float)env->state.ep_max_wave_ticks;
        env->log.max_wave_ticks_wave += (float)env->state.ep_max_wave_ticks_wave;
        env->log.reached_wave_63 += (float)env->state.ep_reached_wave_63;
        env->log.jad_kill_rate += (float)env->state.ep_jad_killed;
        for (int i = 0; i < NPC_TYPE_COUNT; i++) {
            env->log.dmg_to_npc_type[i] +=
                (float)env->state.ep_damage_to_npc_type[i];
            env->log.resolved_hits_to_npc_type[i] +=
                (float)env->state.ep_resolved_hits_to_npc_type[i];
            env->log.damaging_hits_to_npc_type[i] +=
                (float)env->state.ep_damaging_hits_to_npc_type[i];
            env->log.attack_cycles_to_npc_type[i] +=
                (float)env->state.ep_attack_cycles_to_npc_type[i];
            env->log.target_ticks_by_npc_type[i] +=
                (float)env->state.ep_target_ticks_by_npc_type[i];
        }
        env->log.target_held_ticks += (float)env->state.ep_target_held_ticks;
        env->log.no_target_ticks += (float)env->state.ep_no_target_ticks;
        env->log.target_in_range_los_ticks +=
            (float)env->state.ep_target_in_range_los_ticks;
        env->log.target_out_of_range_or_los_ticks +=
            (float)env->state.ep_target_out_of_range_or_los_ticks;
        env->log.attack_cooldown_wait_ticks +=
            (float)env->state.ep_attack_cooldown_wait_ticks;
        env->log.ready_but_no_attack_ticks +=
            (float)env->state.ep_ready_but_no_attack_ticks;
        env->log.action_move_idle_ticks +=
            (float)env->state.ep_action_move_idle_ticks;
        env->log.action_move_walk_ticks +=
            (float)env->state.ep_action_move_walk_ticks;
        env->log.action_move_run_ticks +=
            (float)env->state.ep_action_move_run_ticks;
        env->log.action_attack_none_ticks +=
            (float)env->state.ep_action_attack_none_ticks;
        env->log.action_attack_target_ticks +=
            (float)env->state.ep_action_attack_target_ticks;
        env->log.action_prayer_noop_ticks +=
            (float)env->state.ep_action_prayer_noop_ticks;
        env->log.action_prayer_cmd_ticks +=
            (float)env->state.ep_action_prayer_cmd_ticks;
        env->log.no_progress_ticks += env->ep_no_progress_ticks;
        env->log.no_progress_idle_move_ticks += env->ep_no_progress_idle_move_ticks;
        env->log.no_progress_move_cmd_ticks += env->ep_no_progress_move_cmd_ticks;
        env->log.no_progress_attack_none_ticks += env->ep_no_progress_attack_none_ticks;
        env->log.no_progress_attack_target_ticks += env->ep_no_progress_attack_target_ticks;
        env->log.no_progress_has_target_ticks += env->ep_no_progress_has_target_ticks;
        env->log.no_progress_no_target_ticks += env->ep_no_progress_no_target_ticks;
        env->log.no_progress_prayer_cmd_ticks += env->ep_no_progress_prayer_cmd_ticks;
        env->log.no_progress_invalid_action_ticks += env->ep_no_progress_invalid_action_ticks;
        env->log.required_work_remaining +=
            env->reward_runtime.last_required_work_remaining;
        env->log.required_work_start +=
            env->reward_runtime.required_work_at_wave_start;
        env->log.cave_progress += env->reward_runtime.last_cave_progress;
        env->log.current_wave_progress +=
            env->reward_runtime.last_current_wave_progress;
        env->log.progress_delta += env->ep_progress_delta;
        env->log.progress_reward += env->ep_progress_reward;
        env->log.ticks_since_positive_progress +=
            (float)env->reward_runtime.ticks_since_positive_progress;
        env->log.positive_progress_ticks +=
            (float)env->reward_runtime.positive_progress_ticks;
        env->log.zero_progress_ticks +=
            (float)env->reward_runtime.zero_progress_ticks;
        env->log.negative_progress_ticks +=
            (float)env->reward_runtime.negative_progress_ticks;
        env->log.gross_damage_dealt += env->ep_gross_damage_dealt;
        env->log.net_required_work_removed += env->ep_net_required_work_removed;
        env->log.gross_damage_to_net_progress_ratio +=
            env->ep_gross_damage_dealt /
            ((env->ep_net_required_work_removed > 1.0f)
                ? env->ep_net_required_work_removed : 1.0f);
        env->log.npc_healing_total += env->ep_npc_healing_total;
        env->log.mejkot_healing_total += env->ep_mejkot_healing_total;
        env->log.jad_healing_total += env->ep_jad_healing_total;
        env->log.preclip_reward += env->ep_preclip_reward;
        env->log.postclip_reward += env->ep_postclip_reward;
        env->log.positive_clip_count += env->ep_positive_clip_count;
        env->log.negative_clip_count += env->ep_negative_clip_count;

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
