#include "fc_capi.h"
#include "fc_contracts.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_BATCH_ENVS 4

typedef void* (*BatchCreateFn)(int num_envs);
typedef void (*BatchDestroyFn)(void* batch);
typedef void (*BatchResetFn)(void* batch, unsigned int base_seed);
typedef void (*BatchStepFn)(void* batch,
                            const int* all_actions,
                            float* all_obs,
                            float* all_rewards,
                            int* all_terminals);
typedef void (*BatchGetObsFn)(const void* batch, float* all_obs);

typedef struct {
    BatchCreateFn create;
    BatchDestroyFn destroy;
    BatchResetFn reset;
    BatchStepFn step;
    BatchGetObsFn get_obs;
} BatchApi;

static int load_batch_api(BatchApi* api) {
    union {
        void* symbol;
        BatchCreateFn fn;
    } create = {dlsym(RTLD_DEFAULT, "fc_capi_batch_create")};
    union {
        void* symbol;
        BatchDestroyFn fn;
    } destroy = {dlsym(RTLD_DEFAULT, "fc_capi_batch_destroy")};
    union {
        void* symbol;
        BatchResetFn fn;
    } reset = {dlsym(RTLD_DEFAULT, "fc_capi_batch_reset")};
    union {
        void* symbol;
        BatchStepFn fn;
    } step = {dlsym(RTLD_DEFAULT, "fc_capi_batch_step_flat")};
    union {
        void* symbol;
        BatchGetObsFn fn;
    } get_obs = {dlsym(RTLD_DEFAULT, "fc_capi_batch_get_obs")};

    if (create.symbol == NULL || destroy.symbol == NULL ||
        reset.symbol == NULL || step.symbol == NULL || get_obs.symbol == NULL) {
        fprintf(stderr,
                "FAIL CONTRACT-003: required contiguous C API batch symbol is missing\n");
        return 0;
    }

    api->create = create.fn;
    api->destroy = destroy.fn;
    api->reset = reset.fn;
    api->step = step.fn;
    api->get_obs = get_obs.fn;
    return 1;
}

static int test_dimension_getters(void) {
    static const int expected_dims[FC_NUM_ACTION_HEADS] = {
        17, 9, 8, 3, 2, 65, 65,
    };
    const int actual_dims[FC_NUM_ACTION_HEADS] = {
        fc_capi_move_dim(),
        fc_capi_attack_dim(),
        fc_capi_prayer_dim(),
        fc_capi_eat_dim(),
        fc_capi_drink_dim(),
        fc_capi_move_target_x_dim(),
        fc_capi_move_target_y_dim(),
    };

    if (fc_capi_obs_size() != FC_OBS_SIZE ||
        fc_capi_policy_obs_size() != FC_POLICY_OBS_SIZE ||
        fc_capi_reward_features() != FC_REWARD_FEATURES ||
        fc_capi_action_mask_size() != FC_ACTION_MASK_SIZE ||
        fc_capi_num_action_heads() != FC_NUM_ACTION_HEADS) {
        fprintf(stderr,
                "FAIL CONTRACT-002: C API size/head getters disagree with fc_contracts.h\n");
        return 1;
    }

    for (int head = 0; head < FC_NUM_ACTION_HEADS; head++) {
        if (actual_dims[head] != expected_dims[head] ||
            actual_dims[head] != FC_ACTION_DIMS[head]) {
            fprintf(stderr,
                    "FAIL CONTRACT-002: C API action dim[%d]=%d, expected %d\n",
                    head, actual_dims[head], expected_dims[head]);
            return 1;
        }
    }

    FcEnvCtx* env = fc_capi_create();
    if (env == NULL) {
        fprintf(stderr, "FAIL CONTRACT-002: fc_capi_create returned NULL\n");
        return 1;
    }
    fc_capi_reset(env, 7001U);
    const float* obs = fc_capi_get_obs(env);
    const float* mask = obs + FC_TOTAL_OBS;
    for (int prayer = 0; prayer < FC_PRAYER_DIM; prayer++) {
        if (mask[FC_MASK_PRAYER_START + prayer] != 1.0f) {
            fprintf(stderr,
                    "FAIL CONTRACT-002: C API reset mask rejects prayer action %d\n",
                    prayer);
            fc_capi_destroy(env);
            return 1;
        }
    }

    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[2] = FC_PRAYER_FLICK_MELEE;
    (void)fc_capi_step(env, actions);
    obs = fc_capi_get_obs(env);
    mask = obs + FC_TOTAL_OBS;
    for (int prayer = 0; prayer < FC_PRAYER_DIM; prayer++) {
        if (mask[FC_MASK_PRAYER_START + prayer] != 1.0f) {
            fprintf(stderr,
                    "FAIL CONTRACT-002: C API step mask rejects prayer action %d\n",
                    prayer);
            fc_capi_destroy(env);
            return 1;
        }
    }
    fc_capi_destroy(env);

    printf("PASS CONTRACT-002: C API getters and eight-action mask match shared constants\n");
    return 0;
}

static int canaries_intact(uint64_t before, uint64_t after) {
    return before == UINT64_C(0x13579BDF2468ACE0) &&
           after == UINT64_C(0x0ECA8642FDB97531);
}

static int test_batch_strides(void) {
    const uint64_t before = UINT64_C(0x13579BDF2468ACE0);
    const uint64_t after = UINT64_C(0x0ECA8642FDB97531);
    struct {
        uint64_t before;
        int values[TEST_BATCH_ENVS * FC_NUM_ACTION_HEADS];
        uint64_t after;
    } actions = {before, {0}, after};
    struct {
        uint64_t before;
        float values[TEST_BATCH_ENVS * FC_OBS_SIZE];
        uint64_t after;
    } observations = {before, {0}, after};
    struct {
        uint64_t before;
        float values[TEST_BATCH_ENVS];
        uint64_t after;
    } rewards = {before, {0}, after};
    struct {
        uint64_t before;
        int values[TEST_BATCH_ENVS];
        uint64_t after;
    } terminals = {before, {0}, after};
    float expected_obs[TEST_BATCH_ENVS][FC_OBS_SIZE];
    float expected_rewards[TEST_BATCH_ENVS];
    int expected_terminals[TEST_BATCH_ENVS];
    FcEnvCtx* singles[TEST_BATCH_ENVS] = {0};
    static const int prayer_commands[TEST_BATCH_ENVS] = {
        FC_PRAYER_NO_CHANGE,
        FC_PRAYER_FLICK_MAGIC,
        FC_PRAYER_FLICK_RANGE,
        FC_PRAYER_FLICK_MELEE,
    };
    const unsigned int base_seed = 8100U;
    BatchApi api;
    void* batch = NULL;
    int result = 1;

    if (!load_batch_api(&api)) return 1;
    batch = api.create(TEST_BATCH_ENVS);
    if (batch == NULL) {
        fprintf(stderr, "FAIL CONTRACT-003: batch create returned NULL\n");
        return 1;
    }

    for (int env = 0; env < TEST_BATCH_ENVS; env++) {
        singles[env] = fc_capi_create();
        if (singles[env] == NULL) {
            fprintf(stderr,
                    "FAIL CONTRACT-003: single-env oracle %d allocation failed\n",
                    env);
            goto cleanup;
        }
        fc_capi_reset(singles[env], base_seed + (unsigned int)env);
        memcpy(expected_obs[env], fc_capi_get_obs(singles[env]),
               sizeof(expected_obs[env]));
        for (int i = 0; i < FC_OBS_SIZE; i++) {
            observations.values[env * FC_OBS_SIZE + i] =
                -1000.0f - (float)(env * FC_OBS_SIZE + i);
        }
    }

    api.reset(batch, base_seed);
    api.get_obs(batch, observations.values);
    for (int env = 0; env < TEST_BATCH_ENVS; env++) {
        const float* actual = observations.values + env * FC_OBS_SIZE;
        if (memcmp(actual, expected_obs[env], sizeof(expected_obs[env])) != 0) {
            fprintf(stderr,
                    "FAIL CONTRACT-003: reset observation slice %d is misaligned\n",
                    env);
            goto cleanup;
        }
    }

    for (int env = 0; env < TEST_BATCH_ENVS; env++) {
        int* env_actions = actions.values + env * FC_NUM_ACTION_HEADS;
        env_actions[0] = FC_MOVE_IDLE;
        env_actions[1] = FC_ATTACK_NONE;
        env_actions[2] = prayer_commands[env];
        expected_terminals[env] = fc_capi_step(singles[env], env_actions);
        expected_rewards[env] = fc_capi_get_reward(singles[env]);
        memcpy(expected_obs[env], fc_capi_get_obs(singles[env]),
               sizeof(expected_obs[env]));
        rewards.values[env] = 1000.0f + (float)env;
        terminals.values[env] = 1000 + env;
    }

    api.step(batch, actions.values, observations.values,
             rewards.values, terminals.values);

    for (int env = 0; env < TEST_BATCH_ENVS; env++) {
        const float* actual = observations.values + env * FC_OBS_SIZE;
        if (memcmp(actual, expected_obs[env], sizeof(expected_obs[env])) != 0 ||
            rewards.values[env] != expected_rewards[env] ||
            terminals.values[env] != expected_terminals[env]) {
            fprintf(stderr,
                    "FAIL CONTRACT-003: batch outputs for env %d do not match its single-env oracle\n",
                    env);
            goto cleanup;
        }
    }

    if (!canaries_intact(actions.before, actions.after) ||
        !canaries_intact(observations.before, observations.after) ||
        !canaries_intact(rewards.before, rewards.after) ||
        !canaries_intact(terminals.before, terminals.after)) {
        fprintf(stderr,
                "FAIL CONTRACT-003: C API batch reset/step crossed a caller buffer\n");
        goto cleanup;
    }

    printf("PASS CONTRACT-003: four-env C API batch strides, canaries, rewards, and terminals align\n");
    result = 0;

cleanup:
    for (int env = 0; env < TEST_BATCH_ENVS; env++) {
        fc_capi_destroy(singles[env]);
    }
    api.destroy(batch);
    return result;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <dimension_getters|batch_strides>\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "dimension_getters") == 0) {
        return test_dimension_getters();
    }
    if (strcmp(argv[1], "batch_strides") == 0) {
        return test_batch_strides();
    }
    fprintf(stderr, "unknown parity C API test: %s\n", argv[1]);
    return 2;
}
