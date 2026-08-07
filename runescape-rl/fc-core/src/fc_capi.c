/*
 * fc_capi.c — Plain C shared library API for Python ctypes/cffi loading.
 *
 * This provides the same interface as binding.c but without requiring Python.h.
 * Python loads the .so via ctypes and calls these functions directly.
 *
 * Each function operates on an opaque FcEnvHandle (pointer to FcEnvCtx).
 */

#include "fc_types.h"
#include "fc_contracts.h"
#include "fc_api.h"
#include "fc_capi.h"
#include "fc_npc.h"
#include <stdlib.h>
#include <string.h>

struct FcEnvCtx {
    FcState state;
    float obs[FC_OBS_SIZE];
    int actions[FC_NUM_ACTION_HEADS];
    float reward;
    int terminal;
    /* Episode accumulators for logging */
    float episode_reward;
    int episode_ticks;
};

/* Exported constants for Python to read (avoids magic numbers) */
int fc_capi_obs_size(void) { return FC_OBS_SIZE; }
int fc_capi_policy_obs_size(void) { return FC_POLICY_OBS_SIZE; }
int fc_capi_reward_features(void) { return FC_REWARD_FEATURES; }
int fc_capi_action_mask_size(void) { return FC_ACTION_MASK_SIZE; }
int fc_capi_num_action_heads(void) { return FC_NUM_ACTION_HEADS; }
int fc_capi_move_dim(void) { return FC_MOVE_DIM; }
int fc_capi_attack_dim(void) { return FC_ATTACK_DIM; }
int fc_capi_prayer_dim(void) { return FC_PRAYER_DIM; }
int fc_capi_eat_dim(void) { return FC_EAT_DIM; }
int fc_capi_drink_dim(void) { return FC_DRINK_DIM; }
int fc_capi_move_target_x_dim(void) { return FC_MOVE_TARGET_X_DIM; }
int fc_capi_move_target_y_dim(void) { return FC_MOVE_TARGET_Y_DIM; }

/* Create a new environment context */
FcEnvCtx* fc_capi_create(void) {
    FcEnvCtx* ctx = (FcEnvCtx*)calloc(1, sizeof(FcEnvCtx));
    fc_init(&ctx->state);
    return ctx;
}

/* Destroy */
void fc_capi_destroy(FcEnvCtx* ctx) {
    if (ctx) {
        fc_destroy(&ctx->state);
        free(ctx);
    }
}

/* Reset with seed. Wave 1 NPCs are auto-spawned by fc_reset. */
void fc_capi_reset(FcEnvCtx* ctx, unsigned int seed) {
    fc_reset(&ctx->state, (uint32_t)seed);

    fc_write_obs(&ctx->state, ctx->obs);
    fc_write_mask(&ctx->state, ctx->obs + FC_TOTAL_OBS);
    ctx->reward = 0.0f;
    ctx->terminal = 0;
    ctx->episode_reward = 0.0f;
    ctx->episode_ticks = 0;
}

/* Step with action heads. Returns terminal flag. */
int fc_capi_step(FcEnvCtx* ctx, const int actions[]) {
    for (int i = 0; i < FC_NUM_ACTION_HEADS; i++)
        ctx->actions[i] = actions[i];

    fc_step(&ctx->state, ctx->actions);

    fc_write_obs(&ctx->state, ctx->obs);
    fc_write_mask(&ctx->state, ctx->obs + FC_TOTAL_OBS);

    /* Compute scalar reward from reward features */
    float* rwd = ctx->obs + FC_REWARD_START;
    float reward = 0.0f;
    reward += rwd[FC_RWD_DAMAGE_DEALT] * 1.0f;
    reward += rwd[FC_RWD_DAMAGE_TAKEN] * -1.0f;
    reward += rwd[FC_RWD_NPC_KILL] * 5.0f;
    reward += rwd[FC_RWD_WAVE_CLEAR] * 10.0f;
    reward += rwd[FC_RWD_PLAYER_DEATH] * -20.0f;
    reward += rwd[FC_RWD_CAVE_COMPLETE] * 100.0f;
    reward += rwd[FC_RWD_TICK_PENALTY] * -0.01f;
    ctx->reward = reward;
    ctx->episode_reward += reward;
    ctx->episode_ticks++;

    ctx->terminal = fc_is_terminal(&ctx->state);

    if (ctx->terminal) {
        /* Auto-reset */
        uint32_t new_seed = ctx->state.rng_state;
        fc_capi_reset(ctx, new_seed);
    }

    return ctx->terminal;
}

/* Getters for Python to read observation/reward/terminal */
const float* fc_capi_get_obs(const FcEnvCtx* ctx) { return ctx->obs; }
float fc_capi_get_reward(const FcEnvCtx* ctx) { return ctx->reward; }
int fc_capi_get_terminal(const FcEnvCtx* ctx) { return ctx->terminal; }

/* ======================================================================== */
/* Batch interface — contiguous array of envs                                */
/* ======================================================================== */

/*
 * Batch context: N environments in a contiguous allocation.
 * Python creates one batch, then passes the handle to batch_step/reset.
 * This avoids pointer-to-pointer indirection and per-env ctypes calls.
 */
struct FcBatchCtx {
    int num_envs;
    FcEnvCtx* envs;  /* contiguous array of FcEnvCtx[num_envs] */
};

FcBatchCtx* fc_capi_batch_create(int num_envs) {
    FcBatchCtx* batch = (FcBatchCtx*)calloc(1, sizeof(FcBatchCtx));
    batch->num_envs = num_envs;
    batch->envs = (FcEnvCtx*)calloc((size_t)num_envs, sizeof(FcEnvCtx));
    for (int i = 0; i < num_envs; i++) {
        fc_init(&batch->envs[i].state);
    }
    return batch;
}

void fc_capi_batch_destroy(FcBatchCtx* batch) {
    if (batch) {
        if (batch->envs) {
            for (int i = 0; i < batch->num_envs; i++) {
                fc_destroy(&batch->envs[i].state);
            }
            free(batch->envs);
        }
        free(batch);
    }
}

void fc_capi_batch_reset(FcBatchCtx* batch, unsigned int base_seed) {
    for (int i = 0; i < batch->num_envs; i++) {
        fc_capi_reset(&batch->envs[i], base_seed + (unsigned int)i);
    }
}

/*
 * Batch step: all N envs in one C call.
 *
 * all_actions: flat int32 array [num_envs × FC_NUM_ACTION_HEADS]
 * all_obs:     flat float32 array [num_envs × FC_OBS_SIZE] (written)
 * all_rewards: flat float32 array [num_envs] (written)
 * all_terminals: flat int32 array [num_envs] (written)
 */
void fc_capi_batch_step_flat(FcBatchCtx* batch,
                              const int* all_actions,
                              float* all_obs,
                              float* all_rewards,
                              int* all_terminals) {
    for (int e = 0; e < batch->num_envs; e++) {
        FcEnvCtx* ctx = &batch->envs[e];
        const int* acts = all_actions + e * FC_NUM_ACTION_HEADS;

        fc_capi_step(ctx, acts);

        memcpy(all_obs + e * FC_OBS_SIZE, ctx->obs, sizeof(float) * FC_OBS_SIZE);
        all_rewards[e] = ctx->reward;
        all_terminals[e] = ctx->terminal;
    }
}

/* Read all obs from batch into flat array (for initial reset) */
void fc_capi_batch_get_obs(const FcBatchCtx* batch, float* all_obs) {
    for (int e = 0; e < batch->num_envs; e++) {
        memcpy(all_obs + e * FC_OBS_SIZE, batch->envs[e].obs, sizeof(float) * FC_OBS_SIZE);
    }
}
