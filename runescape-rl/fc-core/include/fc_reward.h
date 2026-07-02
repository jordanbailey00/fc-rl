#ifndef FC_REWARD_H
#define FC_REWARD_H

#include <string.h>

#include "fc_types.h"
#include "fc_contracts.h"
#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"

typedef struct {
    float w_damage_dealt;      /* legacy per-hit damage shaping; active fc_revamp config sets 0 */
    float w_progress;          /* reward per required-work unit removed */
    float w_damage_taken;
    float w_npc_kill;
    float w_wave_clear;
    float w_jad_kill;          /* combined Jad kill + cave complete reward */
    float w_cave_complete;
    float w_player_death;
    float w_correct_jad_prayer;     /* fires only on Jad hits */
    float w_correct_danger_prayer;  /* fires on non-Jad styled hits (melee/ranged/magic) */
    float w_prayer_lost;            /* per prayer point lost from overhead drain or Tz-Kih */
    float w_invalid_action;
    float w_tick_penalty;

    float shape_unnecessary_prayer_penalty;
    float shape_wave_stall_base_penalty;
    float shape_wave_stall_cap;
    float shape_jad_heal_penalty;       /* per Yt-HurKot heal proc that lands on Jad */
    float shape_npc_heal_penalty;       /* per actual NPC heal proc that restores HP */
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
} FcRewardParams;

typedef struct {
    int ticks_since_attack;
    int ticks_in_wave;
    float required_work_at_wave_start;
    float cave_progress_prev;
    float last_required_work_remaining;
    float last_current_wave_progress;
    float last_cave_progress;
    float last_progress_delta;
    float last_progress_reward;
    float last_net_required_work_removed;
    int ticks_since_positive_progress;
    int positive_progress_ticks;
    int zero_progress_ticks;
    int negative_progress_ticks;
} FcRewardRuntime;

typedef struct {
    int melee_pressure_npcs;
    int any_threat;
    int tokxil_melee;
    int ketzek_melee;
} FcRewardThreatContext;

typedef struct {
    float raw[FC_REWARD_FEATURES];

    float damage_dealt;
    float progress;
    float damage_taken;
    float npc_kill;
    float wave_clear;
    float jad_kill;
    float cave_complete;
    float player_death;

    float correct_jad_prayer;
    float correct_danger_prayer;
    float prayer_lost;
    float unnecessary_prayer;
    float wave_stall;
    float no_progress;
    float no_attack;
    float jad_heal;
    float npc_heal;

    float invalid_action;
    float tick_penalty;

    float total;
    FcRewardThreatContext threat_ctx;
} FcRewardBreakdown;

/* Reward channel enumeration — one slot per named field in FcRewardBreakdown above,
 * excluding `raw` and `total`. Used by analytics code in fc-training to accumulate
 * per-channel sums and fire counts per episode. */
typedef enum {
    FC_CH_DAMAGE_DEALT = 0,
    FC_CH_PROGRESS,
    FC_CH_DAMAGE_TAKEN,
    FC_CH_NPC_KILL,
    FC_CH_WAVE_CLEAR,
    FC_CH_JAD_KILL,
    FC_CH_CAVE_COMPLETE,
    FC_CH_PLAYER_DEATH,
    FC_CH_CORRECT_JAD_PRAYER,
    FC_CH_CORRECT_DANGER_PRAYER,
    FC_CH_PRAYER_LOST,
    FC_CH_UNNECESSARY_PRAYER,
    FC_CH_WAVE_STALL,
    FC_CH_NO_PROGRESS,
    FC_CH_NO_ATTACK,
    FC_CH_JAD_HEAL,
    FC_CH_NPC_HEAL,
    FC_CH_INVALID_ACTION,
    FC_CH_TICK_PENALTY,
    FC_CH_COUNT
} FcRwdChannel;

static const char* const FC_CH_NAMES[FC_CH_COUNT] = {
    "damage_dealt", "progress", "damage_taken", "npc_kill", "wave_clear",
    "jad_kill", "cave_complete", "player_death", "correct_jad_prayer",
    "correct_danger_prayer", "prayer_lost", "unnecessary_prayer", "wave_stall",
    "no_progress", "no_attack", "jad_heal", "npc_heal", "invalid_action",
    "tick_penalty"
};

/* Populate a contiguous array view of the breakdown channels for iteration.
 * Order matches FcRwdChannel enum above. */
static inline void fc_reward_breakdown_channels(const FcRewardBreakdown* b, float out[FC_CH_COUNT]) {
    out[FC_CH_DAMAGE_DEALT]             = b->damage_dealt;
    out[FC_CH_PROGRESS]                 = b->progress;
    out[FC_CH_DAMAGE_TAKEN]             = b->damage_taken;
    out[FC_CH_NPC_KILL]                 = b->npc_kill;
    out[FC_CH_WAVE_CLEAR]               = b->wave_clear;
    out[FC_CH_JAD_KILL]                 = b->jad_kill;
    out[FC_CH_CAVE_COMPLETE]            = b->cave_complete;
    out[FC_CH_PLAYER_DEATH]             = b->player_death;
    out[FC_CH_CORRECT_JAD_PRAYER]       = b->correct_jad_prayer;
    out[FC_CH_CORRECT_DANGER_PRAYER]    = b->correct_danger_prayer;
    out[FC_CH_PRAYER_LOST]              = b->prayer_lost;
    out[FC_CH_UNNECESSARY_PRAYER]       = b->unnecessary_prayer;
    out[FC_CH_WAVE_STALL]               = b->wave_stall;
    out[FC_CH_NO_PROGRESS]              = b->no_progress;
    out[FC_CH_NO_ATTACK]                = b->no_attack;
    out[FC_CH_JAD_HEAL]                 = b->jad_heal;
    out[FC_CH_NPC_HEAL]                 = b->npc_heal;
    out[FC_CH_INVALID_ACTION]           = b->invalid_action;
    out[FC_CH_TICK_PENALTY]             = b->tick_penalty;
}

static inline FcRewardParams fc_reward_default_params(void) {
    FcRewardParams params;
    memset(&params, 0, sizeof(params));

    params.w_damage_dealt = 0.0f;
    params.w_progress = 0.001f;
    params.w_damage_taken = -0.25f;
    params.w_npc_kill = 0.0f;
    params.w_wave_clear = 0.0f;
    params.w_jad_kill = 0.0f;
    params.w_cave_complete = 1.0f;
    params.w_player_death = -1.0f;
    params.w_correct_jad_prayer = 0.0f;
    params.w_correct_danger_prayer = 0.005f;
    params.w_prayer_lost = -0.02f;
    params.w_invalid_action = -0.1f;
    params.w_tick_penalty = -0.0001f;

    params.shape_unnecessary_prayer_penalty = 0.0f;
    params.shape_wave_stall_base_penalty = 0.0f;
    params.shape_wave_stall_cap = 0.0f;
    params.shape_jad_heal_penalty = 0.0f;
    params.shape_npc_heal_penalty = 0.0f;
    params.shape_no_progress_penalty_1 = -0.001f;
    params.shape_no_progress_penalty_2 = -0.005f;
    params.shape_no_progress_penalty_3 = -0.02f;
    params.shape_no_attack_base_penalty = -0.005f;
    params.shape_no_attack_wave_scale = 0.05f;

    params.shape_wave_stall_start = 0;
    params.shape_wave_stall_ramp_interval = 0;
    params.shape_no_progress_start_1 = 800;
    params.shape_no_progress_start_2 = 1600;
    params.shape_no_progress_start_3 = 2400;
    params.shape_no_attack_start = 50;

    return params;
}

static inline void fc_reward_runtime_reset(FcRewardRuntime* runtime) {
    memset(runtime, 0, sizeof(*runtime));
}

static inline float fc_reward_clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static inline float fc_reward_required_work_remaining(const FcState* state) {
    if (state->terminal == TERMINAL_CAVE_COMPLETE) {
        return 0.0f;
    }

    float work = 0.0f;
    const FcNpcStats* small_kek_stats = fc_npc_get_stats(NPC_TZ_KEK_SM);

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        const FcNpc* npc = &state->npcs[i];
        if (!npc->active || npc->is_dead) continue;

        if (state->current_wave == FC_NUM_WAVES) {
            if (npc->npc_type == NPC_TZTOK_JAD) {
                work += (float)npc->current_hp;
            }
            continue;
        }

        if (npc->npc_type == NPC_TZ_KEK) {
            work += (float)npc->current_hp +
                    2.0f * (float)small_kek_stats->max_hp;
        } else {
            work += (float)npc->current_hp;
        }
    }

    return (work > 0.0f) ? work : 0.0f;
}

static inline float fc_reward_current_wave_progress(
        const FcState* state, const FcRewardRuntime* runtime,
        float required_work_remaining) {
    if (state->terminal == TERMINAL_CAVE_COMPLETE) {
        return 1.0f;
    }
    if (state->wave_just_cleared && state->terminal == TERMINAL_NONE) {
        return 0.0f;
    }
    if (runtime->required_work_at_wave_start <= 0.0f) {
        return (required_work_remaining <= 0.0f) ? 1.0f : 0.0f;
    }

    return fc_reward_clamp01(
        1.0f - required_work_remaining / runtime->required_work_at_wave_start);
}

static inline float fc_reward_cave_progress(
        const FcState* state, float current_wave_progress) {
    if (state->terminal == TERMINAL_CAVE_COMPLETE) {
        return 1.0f;
    }

    int waves_cleared = state->current_wave - 1;
    if (waves_cleared < 0) waves_cleared = 0;
    if (waves_cleared > FC_NUM_WAVES) waves_cleared = FC_NUM_WAVES;
    return fc_reward_clamp01(
        ((float)waves_cleared + current_wave_progress) / (float)FC_NUM_WAVES);
}

static inline void fc_reward_sync_progress_state(
        FcState* state, const FcRewardRuntime* runtime) {
    state->progress_required_work_start = runtime->required_work_at_wave_start;
    state->progress_required_work_remaining = runtime->last_required_work_remaining;
    state->progress_current_wave_progress = runtime->last_current_wave_progress;
    state->progress_cave_progress = runtime->last_cave_progress;
    state->progress_delta_this_tick = runtime->last_progress_delta;
    state->progress_ticks_since_positive = runtime->ticks_since_positive_progress;
}

static inline void fc_reward_runtime_begin_episode(
        FcRewardRuntime* runtime, FcState* state) {
    fc_reward_runtime_reset(runtime);
    runtime->required_work_at_wave_start = fc_reward_required_work_remaining(state);
    runtime->last_required_work_remaining = runtime->required_work_at_wave_start;
    runtime->last_current_wave_progress = fc_reward_current_wave_progress(
        state, runtime, runtime->last_required_work_remaining);
    runtime->last_cave_progress = fc_reward_cave_progress(
        state, runtime->last_current_wave_progress);
    runtime->cave_progress_prev = runtime->last_cave_progress;
    fc_reward_sync_progress_state(state, runtime);
}

static inline FcRewardThreatContext fc_reward_collect_threat_context(
        const FcState* state) {
    FcRewardThreatContext ctx;
    const FcPlayer* p = &state->player;

    memset(&ctx, 0, sizeof(ctx));

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        const FcNpc* n = &state->npcs[i];
        if (!n->active || n->is_dead) continue;

        int dist = fc_distance_to_npc(p->x, p->y, n);
        if (dist <= 1) {
            ctx.melee_pressure_npcs++;
            if (n->npc_type == NPC_TOK_XIL) ctx.tokxil_melee = 1;
            if (n->npc_type == NPC_KET_ZEK) ctx.ketzek_melee = 1;
        }

        if (dist <= n->attack_range) {
            ctx.any_threat = 1;
        }
    }

    for (int i = 0; i < p->num_pending_hits; i++) {
        const FcPendingHit* ph = &p->pending_hits[i];
        if (!ph->active) continue;
        ctx.any_threat = 1;
    }

    return ctx;
}

static inline FcRewardBreakdown fc_reward_compute_breakdown(
        const FcState* state, const FcRewardParams* params, FcRewardRuntime* runtime) {
    FcRewardBreakdown out;
    const FcPlayer* p = &state->player;
    int prayer_reward_idle;

    memset(&out, 0, sizeof(out));
    fc_write_reward_features(state, out.raw);
    out.threat_ctx = fc_reward_collect_threat_context(state);
    prayer_reward_idle =
        (runtime->ticks_since_attack >= 1 && out.raw[FC_RWD_ATTACK_ATTEMPT] <= 0.0f);

    {
        float work_remaining = fc_reward_required_work_remaining(state);
        float wave_progress = fc_reward_current_wave_progress(
            state, runtime, work_remaining);
        float cave_progress = fc_reward_cave_progress(state, wave_progress);
        float start_work = runtime->required_work_at_wave_start;
        float progress_delta = cave_progress - runtime->cave_progress_prev;
        float net_work_removed = progress_delta * (float)FC_NUM_WAVES *
            ((start_work > 0.0f) ? start_work : 0.0f);

        /* Scalar reward uses raw net required-work removed. The cave-progress
         * delta stays normalized for observations/logs, while this channel pays
         * for actual HP/work removed and goes negative when healing restores
         * work. Multiplying by the wave's start work preserves the existing
         * wave-clear handling and avoids treating the next wave spawn as
         * negative progress. */
        out.progress = net_work_removed * params->w_progress;

        runtime->last_required_work_remaining = work_remaining;
        runtime->last_current_wave_progress = wave_progress;
        runtime->last_cave_progress = cave_progress;
        runtime->last_progress_delta = progress_delta;
        runtime->last_progress_reward = out.progress;
        runtime->last_net_required_work_removed = net_work_removed;

        if (net_work_removed > 0.0001f) {
            runtime->ticks_since_positive_progress = 0;
            runtime->positive_progress_ticks++;
        } else {
            runtime->ticks_since_positive_progress++;
            if (net_work_removed < -0.0001f) {
                runtime->negative_progress_ticks++;
            } else {
                runtime->zero_progress_ticks++;
            }
        }

        if (params->shape_no_progress_start_1 > 0 &&
            runtime->ticks_since_positive_progress > params->shape_no_progress_start_1) {
            out.no_progress += params->shape_no_progress_penalty_1;
        }
        if (params->shape_no_progress_start_2 > 0 &&
            runtime->ticks_since_positive_progress > params->shape_no_progress_start_2) {
            out.no_progress += params->shape_no_progress_penalty_2;
        }
        if (params->shape_no_progress_start_3 > 0 &&
            runtime->ticks_since_positive_progress > params->shape_no_progress_start_3) {
            out.no_progress += params->shape_no_progress_penalty_3;
        }
    }

    /* damage_dealt fires per damaging hit: (damage + damaging_hits) * w.
     * Base reward per hit only applies when actual damage is dealt; zero
     * damage impacts still resolve mechanically but do not pay damage reward. */
    out.damage_dealt = (out.raw[FC_RWD_DAMAGE_DEALT] +
                       (float)state->hits_landed_this_tick) * params->w_damage_dealt;

    {
        float dmg_frac = out.raw[FC_RWD_DAMAGE_TAKEN];
        out.damage_taken = dmg_frac * params->w_damage_taken;
    }

    out.npc_kill = out.raw[FC_RWD_NPC_KILL] * params->w_npc_kill;

    if (out.raw[FC_RWD_WAVE_CLEAR] > 0.0f) {
        int cleared_wave = state->current_wave - 1;
        if (cleared_wave < 1) cleared_wave = 1;
        out.wave_clear = params->w_wave_clear * (float)cleared_wave;
    }

    out.jad_kill = out.raw[FC_RWD_JAD_KILL] * params->w_jad_kill;
    out.cave_complete = out.raw[FC_RWD_CAVE_COMPLETE] * params->w_cave_complete;
    out.player_death = out.raw[FC_RWD_PLAYER_DEATH] * params->w_player_death;

    /* Prayer correctness: Jad and non-Jad are mutually exclusive — the
     * upstream tracking in fc_combat.c guarantees at most one of
     * CORRECT_JAD_PRAY / CORRECT_DANGER_PRAY is set per incoming hit.
     * Non-Jad path covers any styled block (melee/ranged/magic). */
    if (!prayer_reward_idle) {
        out.correct_jad_prayer =
            out.raw[FC_RWD_CORRECT_JAD_PRAY] * params->w_correct_jad_prayer;
        out.correct_danger_prayer =
            out.raw[FC_RWD_CORRECT_DANGER_PRAY] * params->w_correct_danger_prayer;
    }
    out.prayer_lost = out.raw[FC_RWD_PRAYER_LOST] * params->w_prayer_lost;
    if (p->prayer != PRAYER_NONE && !out.threat_ctx.any_threat) {
        out.unnecessary_prayer = params->shape_unnecessary_prayer_penalty;
    }

    out.invalid_action = out.raw[FC_RWD_INVALID_ACTION] * params->w_invalid_action;
    out.tick_penalty = out.raw[FC_RWD_TICK_PENALTY] * params->w_tick_penalty;

    if (out.raw[FC_RWD_ATTACK_ATTEMPT] > 0.0f) {
        runtime->ticks_since_attack = 0;
    } else if (state->npcs_remaining > 0 && p->attack_timer <= 0) {
        runtime->ticks_since_attack++;
    } else if (state->npcs_remaining <= 0) {
        runtime->ticks_since_attack = 0;
    }

    if (params->shape_no_attack_start > 0 &&
        params->shape_no_attack_base_penalty != 0.0f &&
        state->npcs_remaining > 0 &&
        runtime->ticks_since_attack > params->shape_no_attack_start) {
        float wave = (float)state->current_wave;
        if (wave < 1.0f) wave = 1.0f;
        float multiplier = 1.0f +
            params->shape_no_attack_wave_scale * (wave - 1.0f);
        if (multiplier < 0.0f) multiplier = 0.0f;
        out.no_attack = params->shape_no_attack_base_penalty * multiplier;
    }

    /* Wave-stall penalty — timer-based, fires every tick past the threshold
     * while the wave still has NPCs. Ramps linearly and clamps at cap.
     * Runtime timer resets when a wave_clear fires this tick. */
    if (state->npcs_remaining > 0) {
        runtime->ticks_in_wave++;
        if (params->shape_wave_stall_base_penalty != 0.0f &&
            runtime->ticks_in_wave > params->shape_wave_stall_start) {
            int over = runtime->ticks_in_wave - params->shape_wave_stall_start;
            int ramps = (params->shape_wave_stall_ramp_interval > 0)
                ? over / params->shape_wave_stall_ramp_interval : 0;
            float p = params->shape_wave_stall_base_penalty * (1.0f + (float)ramps);
            float cap = params->shape_wave_stall_cap;
            if (cap != 0.0f && p < cap) p = cap;
            out.wave_stall = p;
        }
    }
    if (out.raw[FC_RWD_WAVE_CLEAR] > 0.0f) {
        runtime->ticks_in_wave = 0;
    }

    /* Jad heal penalty: fires per Yt-HurKot heal proc that landed on Jad
     * this tick. Encourages the agent to break healer link or kill healers
     * before they restore Jad's HP. */
    if (state->jad_heal_procs_this_tick > 0 &&
        params->shape_jad_heal_penalty != 0.0f) {
        out.jad_heal = params->shape_jad_heal_penalty *
                       (float)state->jad_heal_procs_this_tick;
    }
    if (state->npc_heal_procs_this_tick > 0 &&
        params->shape_npc_heal_penalty != 0.0f) {
        out.npc_heal = params->shape_npc_heal_penalty *
                       (float)state->npc_heal_procs_this_tick;
    }

    if (state->wave_just_cleared && state->terminal == TERMINAL_NONE) {
        runtime->required_work_at_wave_start =
            fc_reward_required_work_remaining(state);
        runtime->last_required_work_remaining =
            runtime->required_work_at_wave_start;
        runtime->last_current_wave_progress = 0.0f;
        runtime->last_cave_progress =
            fc_reward_cave_progress(state, runtime->last_current_wave_progress);
    }
    runtime->cave_progress_prev = runtime->last_cave_progress;

    out.total =
        out.damage_dealt +
        out.progress +
        out.damage_taken +
        out.npc_kill +
        out.wave_clear +
        out.jad_kill +
        out.cave_complete +
        out.player_death +
        out.correct_jad_prayer +
        out.correct_danger_prayer +
        out.prayer_lost +
        out.unnecessary_prayer +
        out.wave_stall +
        out.no_progress +
        out.no_attack +
        out.jad_heal +
        out.npc_heal +
        out.invalid_action +
        out.tick_penalty;

    return out;
}

#endif /* FC_REWARD_H */
