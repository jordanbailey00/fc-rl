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
    int expected_obs = FC_POLICY_OBS_SIZE + FC_MOVE_DIM + FC_ATTACK_DIM + FC_PRAYER_DIM;

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
        fprintf(stderr, "usage: %s <rng_seed_diversity|no_supplies_policy_contract>\n", argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "rng_seed_diversity") == 0) {
        return test_rng_seed_diversity();
    }
    if (strcmp(argv[1], "no_supplies_policy_contract") == 0) {
        return test_no_supplies_policy_contract();
    }
    fprintf(stderr, "unknown guardrail: %s\n", argv[1]);
    return 2;
}
