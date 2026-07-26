#include "fight_caves.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void init_env(FightCaves* env,
                     float* observations,
                     float* actions,
                     float* rewards,
                     float* terminals,
                     int rng) {
    memset(env, 0, sizeof(*env));
    env->observations = observations;
    env->actions = actions;
    env->rewards = rewards;
    env->terminals = terminals;
    env->num_agents = 1;
    env->rng = rng;
    env->initial_sharks = 0;
    env->initial_prayer_doses = 0;
}

static int test_no_supplies_policy_contract(void) {
    int expected_obs = FC_POLICY_OBS_SIZE + FC_PUFFER_MASK_SIZE;

    if (FC_POLICY_OBS_SIZE != 285 || FC_PUFFER_OBS_SIZE != 316) {
        printf("FAIL: expected expanded policy/Puffer obs sizes 285/316, got %d/%d\n",
               FC_POLICY_OBS_SIZE, FC_PUFFER_OBS_SIZE);
        return 1;
    }
    if (FC_PUFFER_NUM_ATNS != 3) {
        printf("FAIL: expected 3 Puffer action heads, got %d\n", FC_PUFFER_NUM_ATNS);
        return 1;
    }
    if (FC_PUFFER_OBS_SIZE != expected_obs) {
        printf("FAIL: expected Puffer obs size %d, got %d\n",
               expected_obs, FC_PUFFER_OBS_SIZE);
        return 1;
    }

    printf("PASS: no-supplies Puffer contract exposes move/attack/prayer only\n");
    return 0;
}

static int test_native_action_mask_adapter(void) {
    FightCaves env;
    float observations[FC_PUFFER_OBS_SIZE] = {0};
    float actions[FC_PUFFER_NUM_ATNS] = {0};
    float rewards[1] = {0};
    float terminals[1] = {0};
    unsigned char action_mask[FC_PUFFER_MASK_SIZE] = {0};
    const int head_sizes[FC_PUFFER_NUM_ATNS] = FC_PUFFER_ACT_SIZES;

    init_env(&env, observations, actions, rewards, terminals, 3003);
    env.action_mask = action_mask;
    c_reset(&env);

    int offset = 0;
    for (int head = 0; head < FC_PUFFER_NUM_ATNS; head++) {
        int legal = 0;
        for (int action = 0; action < head_sizes[head]; action++) {
            int idx = offset + action;
            float obs_mask = observations[FC_POLICY_OBS_SIZE + idx];
            if (action_mask[idx] != 0 && action_mask[idx] != 1) {
                printf("FAIL: native mask[%d]=%u is not binary\n",
                       idx, (unsigned int)action_mask[idx]);
                return 1;
            }
            if ((float)action_mask[idx] != obs_mask) {
                printf("FAIL: native mask[%d]=%u differs from obs mask %.1f\n",
                       idx, (unsigned int)action_mask[idx], obs_mask);
                return 1;
            }
            legal += action_mask[idx] != 0;
        }
        if (legal == 0) {
            printf("FAIL: action head %d has no legal action\n", head);
            return 1;
        }
        offset += head_sizes[head];
    }

    if (offset != FC_PUFFER_MASK_SIZE ||
        action_mask[FC_MASK_MOVE_START + FC_MOVE_IDLE] != 1 ||
        action_mask[FC_MASK_ATTACK_START + FC_ATTACK_NONE] != 1) {
        printf("FAIL: native mask offsets or required no-op actions are wrong\n");
        return 1;
    }

    for (int prayer = 0; prayer < FC_PRAYER_DIM; prayer++) {
        if (action_mask[FC_MASK_PRAYER_START + prayer] != 1) {
            printf("FAIL: legal prayer action %d was hard-masked\n", prayer);
            return 1;
        }
    }

    actions[0] = FC_MOVE_IDLE;
    actions[1] = FC_ATTACK_NONE;
    actions[2] = FC_PRAYER_NO_CHANGE;
    c_step(&env);
    for (int i = 0; i < FC_PUFFER_MASK_SIZE; i++) {
        if ((float)action_mask[i] != observations[FC_POLICY_OBS_SIZE + i]) {
            printf("FAIL: post-step native mask[%d] drifted from obs mask\n", i);
            return 1;
        }
    }

    printf("PASS: native byte mask matches all three Puffer action heads\n");
    return 0;
}

static int test_rng_seed_diversity(void) {
    FightCaves a;
    FightCaves b;
    float obs_a[FC_PUFFER_OBS_SIZE] = {0};
    float obs_b[FC_PUFFER_OBS_SIZE] = {0};
    float actions_a[FC_PUFFER_NUM_ATNS] = {0};
    float actions_b[FC_PUFFER_NUM_ATNS] = {0};
    float rewards_a[1] = {0};
    float rewards_b[1] = {0};
    float terminals_a[1] = {0};
    float terminals_b[1] = {0};

    init_env(&a, obs_a, actions_a, rewards_a, terminals_a, 1001);
    init_env(&b, obs_b, actions_b, rewards_b, terminals_b, 2002);

    c_reset(&a);
    c_reset(&b);

    if (a.state.rng_seed != b.state.rng_seed) {
        printf("PASS: per-env reset seeds differ (%u vs %u)\n",
               a.state.rng_seed, b.state.rng_seed);
        return 0;
    }

    printf("FAIL: per-env reset seeds are identical (%u vs %u); env.rng is not used\n",
           a.state.rng_seed, b.state.rng_seed);
    return 1;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <rng_seed_diversity|no_supplies_policy_contract|native_action_mask_adapter>\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "rng_seed_diversity") == 0) {
        return test_rng_seed_diversity();
    }
    if (strcmp(argv[1], "no_supplies_policy_contract") == 0) {
        return test_no_supplies_policy_contract();
    }
    if (strcmp(argv[1], "native_action_mask_adapter") == 0) {
        return test_native_action_mask_adapter();
    }
    fprintf(stderr, "unknown guardrail: %s\n", argv[1]);
    return 2;
}
