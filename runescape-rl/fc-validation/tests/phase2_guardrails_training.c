#include "fight_caves.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(FC_NUM_ACTION_HEADS == 7,
               "core action-head count changed unexpectedly");
_Static_assert(FC_PRAYER_DIM == 8,
               "prayer action head must expose commands 0-7");
_Static_assert(FC_ACTION_MASK_SIZE == 169,
               "full action-mask size must follow the eight-action prayer head");
_Static_assert(FC_OBS_SIZE == 474,
               "full core buffer size must follow the expanded mask");
_Static_assert(FC_PUFFER_NUM_ATNS == 3,
               "Puffer policy must retain three action heads");
_Static_assert(FC_PUFFER_MASK_SIZE == 34,
               "Puffer mask must contain 17+9+8 entries");
_Static_assert(FC_PUFFER_OBS_SIZE == 319,
               "Puffer observation must contain 285 features and 34 masks");

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

    if (FC_POLICY_OBS_SIZE != 285 || FC_PUFFER_OBS_SIZE != 319) {
        printf("FAIL: expected expanded policy/Puffer obs sizes 285/319, got %d/%d\n",
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

static int test_prayer_contract_dimensions(void) {
    static const int expected_core[FC_NUM_ACTION_HEADS] = {
        17, 9, 8, 3, 2, 65, 65,
    };
    static const int expected_puffer[FC_PUFFER_NUM_ATNS] = {17, 9, 8};

    if (FC_POLICY_OBS_SIZE != 285 || FC_REWARD_FEATURES != 20 ||
        FC_TOTAL_OBS != 305 || FC_NUM_ACTION_HEADS != 7 ||
        FC_PRAYER_DIM != 8 || FC_ACTION_MASK_SIZE != 169 ||
        FC_OBS_SIZE != 474 || FC_PUFFER_NUM_ATNS != 3 ||
        FC_PUFFER_MASK_SIZE != 34 || FC_PUFFER_OBS_SIZE != 319) {
        fprintf(stderr,
                "FAIL CONTRACT-001: derived sizes do not match the parity contract\n");
        return 1;
    }

    if (FC_MASK_MOVE_START != 0 || FC_MASK_ATTACK_START != 17 ||
        FC_MASK_PRAYER_START != 26 || FC_MASK_EAT_START != 34 ||
        FC_MASK_DRINK_START != 37 || FC_MASK_TARGET_X_START != 39 ||
        FC_MASK_TARGET_Y_START != 104) {
        fprintf(stderr,
                "FAIL CONTRACT-001: full-mask offsets do not match 0/17/26/34/37/39/104\n");
        return 1;
    }

    for (int i = 0; i < FC_NUM_ACTION_HEADS; i++) {
        if (FC_ACTION_DIMS[i] != expected_core[i]) {
            fprintf(stderr,
                    "FAIL CONTRACT-001: core action dim[%d]=%d, expected %d\n",
                    i, FC_ACTION_DIMS[i], expected_core[i]);
            return 1;
        }
    }
    for (int i = 0; i < FC_PUFFER_NUM_ATNS; i++) {
        if (FC_PUFFER_ACTION_DIMS[i] != expected_puffer[i]) {
            fprintf(stderr,
                    "FAIL CONTRACT-001: Puffer action dim[%d]=%d, expected %d\n",
                    i, FC_PUFFER_ACTION_DIMS[i], expected_puffer[i]);
            return 1;
        }
    }

    printf("PASS CONTRACT-001: exact dimensions, sizes, and offsets\n");
    return 0;
}

static int puffer_masks_match(const float* observations,
                              const unsigned char* action_mask,
                              const char* phase) {
    const int head_sizes[FC_PUFFER_NUM_ATNS] = FC_PUFFER_ACT_SIZES;
    int offset = 0;

    for (int head = 0; head < FC_PUFFER_NUM_ATNS; head++) {
        int legal = 0;
        for (int action = 0; action < head_sizes[head]; action++) {
            int idx = offset + action;
            float float_mask = observations[FC_POLICY_OBS_SIZE + idx];
            if ((action_mask[idx] != 0 && action_mask[idx] != 1) ||
                float_mask != (float)action_mask[idx]) {
                fprintf(stderr,
                        "FAIL CONTRACT-002: %s mask[%d] byte/float=%u/%.1f\n",
                        phase, idx, (unsigned int)action_mask[idx], float_mask);
                return 0;
            }
            legal += action_mask[idx] != 0;
        }
        if (legal == 0) {
            fprintf(stderr,
                    "FAIL CONTRACT-002: %s head %d has no legal action\n",
                    phase, head);
            return 0;
        }
        offset += head_sizes[head];
    }

    if (offset != FC_PUFFER_MASK_SIZE) {
        fprintf(stderr,
                "FAIL CONTRACT-002: %s mask consumed %d entries, expected %d\n",
                phase, offset, FC_PUFFER_MASK_SIZE);
        return 0;
    }
    for (int prayer = 0; prayer < FC_PRAYER_DIM; prayer++) {
        if (action_mask[FC_MASK_PRAYER_START + prayer] != 1) {
            fprintf(stderr,
                    "FAIL CONTRACT-002: %s prayer command %d was hard-masked\n",
                    phase, prayer);
            return 0;
        }
    }
    return 1;
}

static int test_prayer_puffer_mask_values(void) {
    FightCaves env;
    float observations[FC_PUFFER_OBS_SIZE] = {0};
    float actions[FC_PUFFER_NUM_ATNS] = {0};
    float rewards[1] = {0};
    float terminals[1] = {0};
    unsigned char action_mask[FC_PUFFER_MASK_SIZE] = {0};

    init_env(&env, observations, actions, rewards, terminals, 4040);
    env.action_mask = action_mask;
    c_reset(&env);
    if (!puffer_masks_match(observations, action_mask, "reset")) return 1;

    env.state.player.current_prayer = 0;
    env.state.player.prayer = PRAYER_NONE;
    for (int command = FC_PRAYER_FLICK_MAGIC;
         command <= FC_PRAYER_FLICK_MELEE; command++) {
        actions[0] = FC_MOVE_IDLE;
        actions[1] = FC_ATTACK_NONE;
        actions[2] = (float)command;
        c_step(&env);
        if (env.state.invalid_action_this_tick ||
            env.state.player.prayer != PRAYER_NONE ||
            !puffer_masks_match(observations, action_mask, "post-step")) {
            fprintf(stderr,
                    "FAIL CONTRACT-002: zero-Prayer command %d was invalid or changed overhead\n",
                    command);
            return 1;
        }
    }

    printf("PASS CONTRACT-002: all eight Puffer prayer masks are valid and synchronized\n");
    return 0;
}

static int test_prayer_single_env_canaries(void) {
    const uint32_t before = UINT32_C(0x13579BDF);
    const uint32_t after = UINT32_C(0x2468ACE0);
    struct {
        uint32_t before;
        float values[FC_PUFFER_OBS_SIZE];
        uint32_t after;
    } observations = {before, {0}, after};
    struct {
        uint32_t before;
        float values[FC_PUFFER_NUM_ATNS];
        uint32_t after;
    } actions = {before, {0}, after};
    struct {
        uint32_t before;
        unsigned char values[FC_PUFFER_MASK_SIZE];
        uint32_t after;
    } masks = {before, {0}, after};
    struct {
        uint32_t before;
        float values[FC_OBS_SIZE];
        uint32_t after;
    } core = {before, {0}, after};
    FightCaves env;
    float rewards[1] = {0};
    float terminals[1] = {0};

    init_env(&env, observations.values, actions.values,
             rewards, terminals, 5050);
    env.action_mask = masks.values;
    c_reset(&env);

    fc_write_obs(&env.state, core.values);
    fc_write_mask(&env.state, core.values + FC_TOTAL_OBS);

    actions.values[0] = FC_MOVE_IDLE;
    actions.values[1] = FC_ATTACK_NONE;
    actions.values[2] = FC_PRAYER_FLICK_MELEE;
    c_step(&env);

    if (observations.before != before || observations.after != after ||
        actions.before != before || actions.after != after ||
        masks.before != before || masks.after != after ||
        core.before != before || core.after != after) {
        fprintf(stderr,
                "FAIL CONTRACT-003: reset/write/step crossed a declared single-env buffer\n");
        return 1;
    }

    printf("PASS CONTRACT-003: core and Puffer single-env canaries intact\n");
    return 0;
}

static int test_native_action_mask_adapter(void) {
    FightCaves env;
    float observations[FC_PUFFER_OBS_SIZE] = {0};
    float actions[FC_PUFFER_NUM_ATNS] = {0};
    float rewards[1] = {0};
    float terminals[1] = {0};
    unsigned char action_mask[FC_PUFFER_MASK_SIZE] = {0};

    init_env(&env, observations, actions, rewards, terminals, 3003);
    env.action_mask = action_mask;
    c_reset(&env);

    if (!puffer_masks_match(observations, action_mask, "adapter reset") ||
        action_mask[FC_MASK_MOVE_START + FC_MOVE_IDLE] != 1 ||
        action_mask[FC_MASK_ATTACK_START + FC_ATTACK_NONE] != 1) {
        printf("FAIL: native mask offsets or required no-op actions are wrong\n");
        return 1;
    }

    actions[0] = FC_MOVE_IDLE;
    actions[1] = FC_ATTACK_NONE;
    actions[2] = FC_PRAYER_NO_CHANGE;
    c_step(&env);
    if (!puffer_masks_match(observations, action_mask, "adapter post-step"))
        return 1;

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

static int test_prayer_metric_publication(void) {
    const float eps = 0.0001f;
    FightCaves env;
    float observations[FC_PUFFER_OBS_SIZE] = {0};
    float actions[FC_PUFFER_NUM_ATNS] = {0};
    float rewards[1] = {0};
    float terminals[1] = {0};
    static const int commands[] = {
        FC_PRAYER_FLICK_MAGIC,
        FC_PRAYER_FLICK_RANGE,
        FC_PRAYER_FLICK_MELEE,
    };

    init_env(&env, observations, actions, rewards, terminals, 6060);
    c_reset(&env);
    memset(env.state.npcs, 0, sizeof(env.state.npcs));
    env.state.npcs_remaining = 1;
    env.state.player.attack_target_idx = -1;
    env.state.player.prayer = PRAYER_NONE;

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        actions[0] = FC_MOVE_IDLE;
        actions[1] = FC_ATTACK_NONE;
        actions[2] = (float)commands[i];
        if (i == 2) env.state.tick = FC_MAX_EPISODE_TICKS;
        c_step(&env);
    }

    const float expected_uptime = 1.0f / 3.0f;
    if (terminals[0] != 1.0f || env.log.n != 1.0f ||
        fabsf(env.log.prayer_uptime_magic - expected_uptime) > eps ||
        fabsf(env.log.prayer_uptime_range - expected_uptime) > eps ||
        fabsf(env.log.prayer_uptime_melee) > eps ||
        env.log.prayer_switches != 3.0f ||
        env.log.action_prayer_cmd_ticks != 3.0f ||
        env.log.no_progress_prayer_cmd_ticks != 2.0f ||
        env.log.invalid_prayer != 0.0f ||
        env.log.no_progress_invalid_action_ticks != 0.0f ||
        env.log.rwd_sum[FC_CH_INVALID_ACTION] != 0.0f ||
        env.log.rwd_fires[FC_CH_INVALID_ACTION] != 0.0f) {
        fprintf(stderr,
                "FAIL METRIC-002: Puffer prayer uptime/switch/cmd/invalid publication=%.3f/%.3f/%.3f %.0f/%.0f/%.0f %.0f/%.0f %.3f/%.0f\n",
                env.log.prayer_uptime_magic,
                env.log.prayer_uptime_range,
                env.log.prayer_uptime_melee,
                env.log.prayer_switches,
                env.log.action_prayer_cmd_ticks,
                env.log.no_progress_prayer_cmd_ticks,
                env.log.invalid_prayer,
                env.log.no_progress_invalid_action_ticks,
                env.log.rwd_sum[FC_CH_INVALID_ACTION],
                env.log.rwd_fires[FC_CH_INVALID_ACTION]);
        return 1;
    }

    printf("PASS METRIC-002: Puffer publishes tick-start uptime and flick diagnostics\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <rng_seed_diversity|no_supplies_policy_contract|native_action_mask_adapter|prayer_contract_dimensions|prayer_puffer_mask_values|prayer_single_env_canaries|prayer_metric_publication>\n", argv[0]);
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
    if (strcmp(argv[1], "prayer_contract_dimensions") == 0) {
        return test_prayer_contract_dimensions();
    }
    if (strcmp(argv[1], "prayer_puffer_mask_values") == 0) {
        return test_prayer_puffer_mask_values();
    }
    if (strcmp(argv[1], "prayer_single_env_canaries") == 0) {
        return test_prayer_single_env_canaries();
    }
    if (strcmp(argv[1], "prayer_metric_publication") == 0) {
        return test_prayer_metric_publication();
    }
    fprintf(stderr, "unknown guardrail: %s\n", argv[1]);
    return 2;
}
