#ifndef FC_REWARD_H
#define FC_REWARD_H

#include <string.h>

#include "fc_types.h"
#include "fc_contracts.h"
#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"

typedef struct {
    float w_damage_dealt;      /* applied as (damage + damaging_hits) * w per tick */
    float w_damage_taken;
    float w_npc_kill;
    float w_wave_clear;
    float w_jad_kill;          /* combined Jad kill + cave complete reward */
    float w_player_death;
    float w_correct_jad_prayer;     /* fires only on Jad hits */
    float w_correct_danger_prayer;  /* fires on non-Jad styled hits (melee/ranged/magic) */
    float w_invalid_action;
    float w_tick_penalty;

    float shape_unnecessary_prayer_penalty;
    float shape_wave_stall_base_penalty;
    float shape_wave_stall_cap;
    float shape_jad_heal_penalty;       /* per Yt-HurKot heal proc that lands on Jad */

    int shape_wave_stall_start;
    int shape_wave_stall_ramp_interval;
} FcRewardParams;

typedef struct {
    int ticks_since_attack;
    int ticks_in_wave;
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
    float damage_taken;
    float npc_kill;
    float wave_clear;
    float jad_kill;
    float player_death;

    float correct_jad_prayer;
    float correct_danger_prayer;
    float unnecessary_prayer;
    float wave_stall;
    float jad_heal;

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
    FC_CH_DAMAGE_TAKEN,
    FC_CH_NPC_KILL,
    FC_CH_WAVE_CLEAR,
    FC_CH_JAD_KILL,
    FC_CH_PLAYER_DEATH,
    FC_CH_CORRECT_JAD_PRAYER,
    FC_CH_CORRECT_DANGER_PRAYER,
    FC_CH_UNNECESSARY_PRAYER,
    FC_CH_WAVE_STALL,
    FC_CH_JAD_HEAL,
    FC_CH_INVALID_ACTION,
    FC_CH_TICK_PENALTY,
    FC_CH_COUNT
} FcRwdChannel;

static const char* const FC_CH_NAMES[FC_CH_COUNT] = {
    "damage_dealt", "damage_taken", "npc_kill", "wave_clear", "jad_kill",
    "player_death", "correct_jad_prayer", "correct_danger_prayer",
    "unnecessary_prayer", "wave_stall", "jad_heal", "invalid_action",
    "tick_penalty"
};

/* Populate a contiguous array view of the breakdown channels for iteration.
 * Order matches FcRwdChannel enum above. */
static inline void fc_reward_breakdown_channels(const FcRewardBreakdown* b, float out[FC_CH_COUNT]) {
    out[FC_CH_DAMAGE_DEALT]             = b->damage_dealt;
    out[FC_CH_DAMAGE_TAKEN]             = b->damage_taken;
    out[FC_CH_NPC_KILL]                 = b->npc_kill;
    out[FC_CH_WAVE_CLEAR]               = b->wave_clear;
    out[FC_CH_JAD_KILL]                 = b->jad_kill;
    out[FC_CH_PLAYER_DEATH]             = b->player_death;
    out[FC_CH_CORRECT_JAD_PRAYER]       = b->correct_jad_prayer;
    out[FC_CH_CORRECT_DANGER_PRAYER]    = b->correct_danger_prayer;
    out[FC_CH_UNNECESSARY_PRAYER]       = b->unnecessary_prayer;
    out[FC_CH_WAVE_STALL]               = b->wave_stall;
    out[FC_CH_JAD_HEAL]                 = b->jad_heal;
    out[FC_CH_INVALID_ACTION]           = b->invalid_action;
    out[FC_CH_TICK_PENALTY]             = b->tick_penalty;
}

static inline FcRewardParams fc_reward_default_params(void) {
    FcRewardParams params;
    memset(&params, 0, sizeof(params));

    params.w_damage_dealt = 0.5f;
    params.w_damage_taken = -0.6f;
    params.w_npc_kill = 3.0f;
    params.w_wave_clear = 10.0f;
    params.w_jad_kill = 150.0f;
    params.w_player_death = -20.0f;
    params.w_correct_jad_prayer = 0.0f;
    params.w_correct_danger_prayer = 0.25f;
    params.w_invalid_action = -0.1f;
    params.w_tick_penalty = -0.005f;

    params.shape_unnecessary_prayer_penalty = 0.0f;
    params.shape_wave_stall_base_penalty = 0.0f;
    params.shape_wave_stall_cap = 0.0f;
    params.shape_jad_heal_penalty = -0.3f;

    params.shape_wave_stall_start = 0;
    params.shape_wave_stall_ramp_interval = 0;

    return params;
}

static inline void fc_reward_runtime_reset(FcRewardRuntime* runtime) {
    runtime->ticks_since_attack = 0;
    runtime->ticks_in_wave = 0;
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

    /* damage_dealt fires per damaging hit: (damage + damaging_hits) * w.
     * Base reward per hit only applies when actual damage is dealt; zero
     * damage impacts still resolve mechanically but do not pay damage reward. */
    out.damage_dealt = (out.raw[FC_RWD_DAMAGE_DEALT] +
                       (float)state->hits_landed_this_tick) * params->w_damage_dealt;

    {
        float dmg_frac = out.raw[FC_RWD_DAMAGE_TAKEN];
        out.damage_taken = dmg_frac * dmg_frac * 70.0f * params->w_damage_taken;
    }

    out.npc_kill = out.raw[FC_RWD_NPC_KILL] * params->w_npc_kill;

    if (out.raw[FC_RWD_WAVE_CLEAR] > 0.0f) {
        int cleared_wave = state->current_wave - 1;
        if (cleared_wave < 1) cleared_wave = 1;
        out.wave_clear = params->w_wave_clear * (float)cleared_wave;
    }

    out.jad_kill = out.raw[FC_RWD_JAD_KILL] * params->w_jad_kill;
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

    out.total =
        out.damage_dealt +
        out.damage_taken +
        out.npc_kill +
        out.wave_clear +
        out.jad_kill +
        out.player_death +
        out.correct_jad_prayer +
        out.correct_danger_prayer +
        out.unnecessary_prayer +
        out.wave_stall +
        out.jad_heal +
        out.invalid_action +
        out.tick_penalty;

    return out;
}

#endif /* FC_REWARD_H */
