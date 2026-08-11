/*
 * test_headless.c — Phase 9b headless backend verification.
 *
 * Runs the FC backend without any viewer/Raylib dependency. Verifies:
 *   1. Observation values are normalized to [0,1]
 *   2. Reward features fire at correct times
 *   3. Action mask correctly blocks invalid actions
 *   4. Determinism: same seed + actions = identical state hash
 *   5. Multi-episode stability (no crashes, no memory issues)
 *
 * Build: gcc -o test_headless test_headless.c ../src/fc_*.c -I../src -lm
 * Run:   ./test_headless
 */

#include "fc_types.h"
#include "fc_contracts.h"
#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_pathfinding.h"
#include "fc_player_init.h"
#include "fc_reward.h"
#include "fc_wave.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { tests_run++; printf("  TEST: %-50s ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf("FAIL — %s\n", msg); } while(0)

/* Simple seeded RNG for action selection (separate from game RNG) */
static uint32_t test_rng = 12345;
static int test_rand(int max) {
    test_rng ^= test_rng << 13;
    test_rng ^= test_rng >> 17;
    test_rng ^= test_rng << 5;
    return (int)(test_rng % (uint32_t)max);
}

static void init_manual_test_state(FcState* state) {
    fc_init(state);
    memset(state->walkable, 1, sizeof(state->walkable));
    memset(state->los_flags, 0, sizeof(state->los_flags));

    state->player.x = 10;
    state->player.y = 10;
    state->player.max_hp = FC_PLAYER_MAX_HP;
    state->player.current_hp = FC_PLAYER_MAX_HP;
    state->player.max_prayer = FC_PLAYER_MAX_PRAYER;
    state->player.current_prayer = FC_PLAYER_MAX_PRAYER;
    state->player.defence_level = FC_PLAYER_DEFENCE_LVL;
    state->player.magic_level = FC_PLAYER_MAGIC_LVL;
    state->player.defence_crush = FC_EQUIP_DEF_CRUSH;
    state->player.defence_magic = FC_EQUIP_DEF_MAGIC;
    state->player.defence_ranged = FC_EQUIP_DEF_RANGED;
    state->player.ranged_level = FC_PLAYER_RANGED_LVL;
    state->player.weapon_uses_ammo = FC_PLAYER_WEAPON_USES_AMMO;
    state->player.weapon_speed = FC_PLAYER_WEAPON_SPEED;
    state->player.weapon_range = FC_PLAYER_WEAPON_RANGE;
    state->player.ranged_attack_bonus = FC_EQUIP_RANGED_ATK;
    state->player.ranged_strength_bonus = FC_EQUIP_RANGED_STR;
    state->player.attack_target_idx = -1;
}

static void clear_manual_tick_signals(FcState* state) {
    state->correct_jad_prayer = 0;
    state->wrong_jad_prayer = 0;
    state->correct_danger_prayer = 0;
    state->wrong_danger_prayer = 0;
    state->attack_attempt_this_tick = 0;
    state->jad_heal_procs_this_tick = 0;

    state->player.hit_source_npc_type = 0;
    state->player.hit_locked_prayer_this_tick = 0;
    state->player.hit_blocked_this_tick = 0;
    state->player.hit_landed_this_tick = 0;
}

static void apply_test_hp_deficit(FcState* state) {
    int deficit = 250;  /* Keep the player damaged after one shark heal. */
    if (state->player.max_hp <= deficit)
        deficit = state->player.max_hp / 2;
    if (deficit < 10)
        deficit = 10;
    state->player.current_hp = state->player.max_hp - deficit;
    if (state->player.current_hp < 1)
        state->player.current_hp = 1;
    state->player.food_timer = 0;
    state->player.sharks_remaining = FC_MAX_SHARKS;
}

/* ====================================================================== */
/* Test 1: Observation normalization                                       */
/* ====================================================================== */

static void test_obs_normalized(void) {
    printf("\n=== Observation Normalization ===\n");

    FcState state;
    fc_init(&state);
    fc_reset(&state, 42);

    float obs[FC_OBS_SIZE];
    fc_write_obs(&state, obs);

    /* Check all policy obs are in [0, 1] */
    TEST("Policy obs values in [0,1]");
    int all_valid = 1;
    char err[128] = "";
    for (int i = 0; i < FC_POLICY_OBS_SIZE; i++) {
        if (obs[i] < -0.01f || obs[i] > 1.01f) {
            snprintf(err, sizeof(err), "obs[%d] = %.4f (out of range)", i, obs[i]);
            all_valid = 0;
            break;
        }
    }
    if (all_valid) PASS(); else FAIL(err);

    /* Check player HP is correct */
    TEST("Player HP obs = 1.0 at full health");
    if (fabsf(obs[FC_OBS_PLAYER_HP] - 1.0f) < 0.01f) PASS();
    else { snprintf(err, sizeof(err), "got %.4f", obs[FC_OBS_PLAYER_HP]); FAIL(err); }

    /* Check player prayer is correct */
    TEST("Player prayer obs = 1.0 at full prayer");
    if (fabsf(obs[FC_OBS_PLAYER_PRAYER] - 1.0f) < 0.01f) PASS();
    else { snprintf(err, sizeof(err), "got %.4f", obs[FC_OBS_PLAYER_PRAYER]); FAIL(err); }

    /* Check wave obs */
    TEST("Wave obs > 0 (wave 1 started)");
    float wave_obs = obs[FC_OBS_META_START + FC_OBS_META_WAVE];
    if (wave_obs > 0.0f) PASS();
    else { snprintf(err, sizeof(err), "got %.4f", wave_obs); FAIL(err); }

    /* Check sharks obs */
    TEST("Sharks obs = 1.0 (20/20)");
    if (fabsf(obs[FC_OBS_PLAYER_SHARKS] - 1.0f) < 0.01f) PASS();
    else { snprintf(err, sizeof(err), "got %.4f", obs[FC_OBS_PLAYER_SHARKS]); FAIL(err); }

    /* Run a few ticks and check obs still valid */
    int actions[FC_NUM_ACTION_HEADS] = {0};
    for (int t = 0; t < 20; t++) {
        fc_step(&state, actions);
    }
    fc_write_obs(&state, obs);

    TEST("Policy obs still in [0,1] after 20 ticks");
    all_valid = 1;
    for (int i = 0; i < FC_POLICY_OBS_SIZE; i++) {
        if (obs[i] < -0.01f || obs[i] > 1.01f) {
            snprintf(err, sizeof(err), "obs[%d] = %.4f after 20 ticks", i, obs[i]);
            all_valid = 0;
            break;
        }
    }
    if (all_valid) PASS(); else FAIL(err);

    /* Check NPC slots populated (wave 1 has NPCs) */
    TEST("At least 1 NPC visible in observation");
    float npc0_valid = obs[FC_OBS_NPC_START + FC_NPC_VALID];
    if (npc0_valid > 0.5f) PASS();
    else { snprintf(err, sizeof(err), "npc slot 0 valid = %.4f", npc0_valid); FAIL(err); }

    fc_destroy(&state);
}

/* ====================================================================== */
/* Test 2: Reward features                                                 */
/* ====================================================================== */

static void test_reward_features(void) {
    printf("\n=== Reward Features ===\n");

    FcState state;
    fc_init(&state);
    fc_reset(&state, 100);

    float obs[FC_OBS_SIZE];
    int actions[FC_NUM_ACTION_HEADS] = {0};
    char err[128];

    /* Tick penalty should fire every tick */
    fc_step(&state, actions);
    fc_write_obs(&state, obs);
    float* rwd = obs + FC_REWARD_START;

    TEST("Tick penalty fires every tick");
    if (rwd[FC_RWD_TICK_PENALTY] > 0.0f) PASS();
    else { snprintf(err, sizeof(err), "tick_penalty = %.4f", rwd[FC_RWD_TICK_PENALTY]); FAIL(err); }

    /* At reset, no damage/kills should be flagged */
    TEST("No damage dealt at tick 1");
    if (rwd[FC_RWD_DAMAGE_DEALT] < 0.01f) PASS();
    else { snprintf(err, sizeof(err), "damage_dealt = %.4f", rwd[FC_RWD_DAMAGE_DEALT]); FAIL(err); }

    TEST("No player death at tick 1");
    if (rwd[FC_RWD_PLAYER_DEATH] < 0.01f) PASS();
    else FAIL("player_death fired at tick 1");

    /* Queue a deterministic hit so this test is independent of active loadout tankiness. */
    state.player.prayer = PRAYER_NONE;
    state.player.num_pending_hits = 0;
    memset(state.player.pending_hits, 0, sizeof(state.player.pending_hits));
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, state.player.x + 3, state.player.y, 0);
    fc_queue_pending_hit(state.player.pending_hits, &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 30, 1, ATTACK_RANGED, 0, 0);
    memset(actions, 0, sizeof(actions));
    fc_step(&state, actions);
    fc_write_obs(&state, obs);
    rwd = obs + FC_REWARD_START;

    TEST("Damage taken reward fires when NPC hits player");
    if (rwd[FC_RWD_DAMAGE_TAKEN] > 0.0f) PASS();
    else FAIL("queued hit did not produce damage_taken reward");

    /* Test food reward: eat a shark */
    if (state.terminal == TERMINAL_NONE && state.player.current_hp < state.player.max_hp) {
        memset(actions, 0, sizeof(actions));
        actions[3] = FC_EAT_SHARK;
        fc_step(&state, actions);
        fc_write_obs(&state, obs);
        rwd = obs + FC_REWARD_START;

        TEST("Food used reward fires on shark eat");
        if (rwd[FC_RWD_FOOD_USED] > 0.0f) PASS();
        else FAIL("food_used did not fire");
    } else {
        TEST("Food used reward fires on shark eat");
        FAIL("skipped — player dead or full HP");
    }

    fc_destroy(&state);
}

/* ====================================================================== */
/* Test 3: Action mask                                                     */
/* ====================================================================== */

static void test_action_mask(void) {
    printf("\n=== Action Mask ===\n");

    FcState state;
    fc_init(&state);
    fc_reset(&state, 55);

    float mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(&state, mask);

    /* Idle is always valid */
    TEST("Move idle (0) always valid");
    if (mask[FC_MASK_MOVE_START + 0] > 0.5f) PASS(); else FAIL("idle masked");

    /* Attack none is always valid */
    TEST("Attack none (0) always valid");
    if (mask[FC_MASK_ATTACK_START + 0] > 0.5f) PASS(); else FAIL("attack none masked");

    /* Prayer no-change always valid */
    TEST("Prayer no-change (0) always valid");
    if (mask[FC_MASK_PRAYER_START + 0] > 0.5f) PASS(); else FAIL("prayer no-change masked");

    /* Eat shark should be masked at full HP */
    TEST("Eat shark masked at full HP");
    if (state.player.current_hp >= state.player.max_hp) {
        if (mask[FC_MASK_EAT_START + FC_EAT_SHARK] < 0.5f) PASS();
        else FAIL("eat shark NOT masked at full HP");
    } else {
        FAIL("player not at full HP at reset");
    }

    /* Drink ppot should be masked at full prayer */
    TEST("Drink ppot masked at full prayer");
    if (state.player.current_prayer >= state.player.max_prayer) {
        if (mask[FC_MASK_DRINK_START + FC_DRINK_PRAYER_POT] < 0.5f) PASS();
        else FAIL("drink ppot NOT masked at full prayer");
    } else {
        FAIL("player not at full prayer at reset");
    }

    /* Walk-to-tile no-op always valid */
    TEST("Walk-to-tile X no-op (0) always valid");
    if (mask[FC_MASK_TARGET_X_START + 0] > 0.5f) PASS(); else FAIL("target x no-op masked");

    TEST("Walk-to-tile Y no-op (0) always valid");
    if (mask[FC_MASK_TARGET_Y_START + 0] > 0.5f) PASS(); else FAIL("target y no-op masked");

    /* After taking damage, eat should become valid. Make this loadout-independent. */
    int actions[FC_NUM_ACTION_HEADS] = {0};
    apply_test_hp_deficit(&state);

    if (state.player.current_hp < state.player.max_hp && state.terminal == TERMINAL_NONE) {
        fc_write_mask(&state, mask);
        TEST("Eat shark valid after taking damage");
        if (mask[FC_MASK_EAT_START + FC_EAT_SHARK] > 0.5f) PASS();
        else FAIL("eat shark still masked after damage");
    } else {
        TEST("Eat shark valid after taking damage");
        FAIL("skipped — could not get player damaged");
    }

    /* After eating, food timer should mask the eat action */
    if (state.terminal == TERMINAL_NONE && state.player.current_hp < state.player.max_hp) {
        actions[3] = FC_EAT_SHARK;
        fc_step(&state, actions);
        fc_write_mask(&state, mask);

        TEST("Eat shark masked during food cooldown");
        if (mask[FC_MASK_EAT_START + FC_EAT_SHARK] < 0.5f) PASS();
        else FAIL("eat shark NOT masked during cooldown");
    } else {
        TEST("Eat shark masked during food cooldown");
        FAIL("skipped");
    }

    fc_destroy(&state);
}

/* ====================================================================== */
/* Test 4: Determinism                                                     */
/* ====================================================================== */

static void test_determinism(void) {
    printf("\n=== Determinism ===\n");

    uint32_t seed = 777;
    int num_ticks = 500;
    char err[128];

    /* Run 1: record action sequence and state hashes */
    int* recorded_actions = (int*)malloc(sizeof(int) * num_ticks * FC_NUM_ACTION_HEADS);
    uint32_t* hashes1 = (uint32_t*)malloc(sizeof(uint32_t) * num_ticks);

    FcState state;
    fc_init(&state);
    fc_reset(&state, seed);

    test_rng = 99999;  /* reset test RNG */
    for (int t = 0; t < num_ticks; t++) {
        /* Generate random valid actions */
        int actions[FC_NUM_ACTION_HEADS];
        for (int h = 0; h < FC_NUM_ACTION_HEADS; h++) {
            actions[h] = test_rand(FC_ACTION_DIMS[h]);
        }
        /* Mostly idle to avoid dying too fast */
        if (test_rand(3) != 0) actions[0] = 0;
        actions[5] = 0; actions[6] = 0;  /* no walk-to-tile for determinism */

        memcpy(recorded_actions + t * FC_NUM_ACTION_HEADS, actions, sizeof(actions));
        fc_step(&state, actions);
        hashes1[t] = fc_state_hash(&state);

        if (state.terminal != TERMINAL_NONE) {
            num_ticks = t + 1;
            break;
        }
    }
    fc_destroy(&state);

    /* Run 2: replay same seed + same actions, compare hashes */
    fc_init(&state);
    fc_reset(&state, seed);

    int match = 1;
    int mismatch_tick = -1;
    for (int t = 0; t < num_ticks; t++) {
        int* actions = recorded_actions + t * FC_NUM_ACTION_HEADS;
        fc_step(&state, actions);
        uint32_t h2 = fc_state_hash(&state);
        if (h2 != hashes1[t]) {
            match = 0;
            mismatch_tick = t;
            break;
        }
    }
    fc_destroy(&state);

    TEST("Deterministic replay (same seed + actions = same hashes)");
    if (match) {
        snprintf(err, sizeof(err), "matched all %d ticks", num_ticks);
        printf("PASS — %s\n", err);
        tests_passed++;
        tests_run++;
    } else {
        /* already counted by TEST macro... redo */
        snprintf(err, sizeof(err), "mismatch at tick %d", mismatch_tick);
        FAIL(err);
    }
    /* Fix double-count from TEST macro above */
    tests_run--;

    free(recorded_actions);
    free(hashes1);
}

/* ====================================================================== */
/* Test 5: Multi-episode stability                                         */
/* ====================================================================== */

static void test_multi_episode(void) {
    printf("\n=== Multi-Episode Stability ===\n");

    FcState state;
    fc_init(&state);
    char err[128];

    int episodes = 10;
    int max_ticks_per_ep = 500;
    int total_ticks = 0;
    int max_wave = 0;

    for (int ep = 0; ep < episodes; ep++) {
        fc_reset(&state, (uint32_t)(ep + 1));
        test_rng = (uint32_t)(ep * 31337);

        for (int t = 0; t < max_ticks_per_ep; t++) {
            int actions[FC_NUM_ACTION_HEADS] = {0};
            /* Random actions with bias toward useful ones */
            actions[0] = test_rand(FC_MOVE_DIM);
            if (test_rand(3) == 0) actions[1] = test_rand(FC_ATTACK_DIM);
            if (test_rand(5) == 0) actions[2] = test_rand(FC_PRAYER_DIM);
            if (test_rand(8) == 0) actions[3] = test_rand(FC_EAT_DIM);
            if (test_rand(8) == 0) actions[4] = test_rand(FC_DRINK_DIM);

            fc_step(&state, actions);
            total_ticks++;

            if (state.terminal != TERMINAL_NONE) break;
        }

        if (state.current_wave > max_wave) max_wave = state.current_wave;
    }

    TEST("10 episodes complete without crash");
    snprintf(err, sizeof(err), "%d total ticks, max wave %d", total_ticks, max_wave);
    printf("PASS — %s\n", err);
    tests_passed++;

    /* Verify obs/mask still valid after many episodes */
    fc_reset(&state, 999);
    float obs[FC_OBS_SIZE];
    fc_write_obs(&state, obs);
    fc_write_mask(&state, obs + FC_TOTAL_OBS);

    TEST("Obs buffer valid after 10 episodes");
    int valid = 1;
    for (int i = 0; i < FC_POLICY_OBS_SIZE; i++) {
        if (obs[i] < -0.01f || obs[i] > 1.01f || isnan(obs[i])) {
            valid = 0;
            break;
        }
    }
    if (valid) PASS(); else FAIL("obs out of range or NaN");

    fc_destroy(&state);
}

/* ====================================================================== */
/* Test 6: Melee safespot gating                                           */
/* ====================================================================== */

static void test_melee_safespot_gating(void) {
    printf("\n=== Melee Safespot Gating ===\n");

    char err[128];
    float obs[FC_OBS_SIZE];

    FcState blocked;
    init_manual_test_state(&blocked);
    blocked.walkable[10][11] = 0;
    blocked.walkable[11][10] = 0;

    fc_npc_spawn(&blocked.npcs[0], NPC_YT_MEJKOT, 11, 11, 0);
    fc_npc_spawn(&blocked.npcs[1], NPC_TZ_KIH, 15, 11, 1);
    blocked.npcs[0].attack_timer = 0;
    blocked.npcs[0].heal_timer = 0;
    blocked.npcs[1].current_hp = blocked.npcs[1].max_hp / 4;

    fc_npc_tick(&blocked, 0);
    fc_write_obs(&blocked, obs);

    TEST("Safespotted Yt-MejKot does not queue melee hits");
    if (blocked.player.num_pending_hits == 0) PASS();
    else {
        snprintf(err, sizeof(err), "queued %d pending hits", blocked.player.num_pending_hits);
        FAIL(err);
    }

    TEST("Safespotted Yt-MejKot does not heal nearby NPCs");
    if (blocked.npcs[1].current_hp == blocked.npcs[1].max_hp / 4) PASS();
    else {
        snprintf(err, sizeof(err), "healed target to %d", blocked.npcs[1].current_hp);
        FAIL(err);
    }

    TEST("Safespotted melee NPC telegraphs MELEE (distance-based, not LOS-gated)");
    if (obs[FC_OBS_NPC_START + FC_NPC_TELE_MELEE] > 0.5f &&
        obs[FC_OBS_NPC_START + FC_NPC_TELE_RANGED] < 0.5f &&
        obs[FC_OBS_NPC_START + FC_NPC_TELE_MAGIC] < 0.5f) PASS();
    else {
        snprintf(err, sizeof(err), "tele M/R/A = %.2f/%.2f/%.2f",
                 obs[FC_OBS_NPC_START + FC_NPC_TELE_MELEE],
                 obs[FC_OBS_NPC_START + FC_NPC_TELE_RANGED],
                 obs[FC_OBS_NPC_START + FC_NPC_TELE_MAGIC]);
        FAIL(err);
    }

    fc_destroy(&blocked);

    FcState open;
    init_manual_test_state(&open);
    fc_npc_spawn(&open.npcs[0], NPC_YT_MEJKOT, 11, 11, 0);
    fc_npc_spawn(&open.npcs[1], NPC_TZ_KIH, 15, 11, 1);
    open.npcs[0].attack_timer = 0;
    open.npcs[0].heal_timer = 0;
    open.npcs[1].current_hp = open.npcs[1].max_hp / 4;

    fc_npc_tick(&open, 0);
    fc_write_obs(&open, obs);

    TEST("Open-corner Yt-MejKot heal replaces its melee swing");
    if (open.player.num_pending_hits == 0) PASS();
    else {
        snprintf(err, sizeof(err), "queued %d pending hits", open.player.num_pending_hits);
        FAIL(err);
    }

    TEST("Open-corner Yt-MejKot still heals nearby NPCs");
    if (open.npcs[1].current_hp > open.npcs[1].max_hp / 4) PASS();
    else FAIL("expected heal tick");

    TEST("Open-corner melee NPC telegraphs MELEE");
    if (obs[FC_OBS_NPC_START + FC_NPC_TELE_MELEE] > 0.5f &&
        obs[FC_OBS_NPC_START + FC_NPC_TELE_RANGED] < 0.5f &&
        obs[FC_OBS_NPC_START + FC_NPC_TELE_MAGIC] < 0.5f) PASS();
    else {
        snprintf(err, sizeof(err), "tele M/R/A = %.2f/%.2f/%.2f",
                 obs[FC_OBS_NPC_START + FC_NPC_TELE_MELEE],
                 obs[FC_OBS_NPC_START + FC_NPC_TELE_RANGED],
                 obs[FC_OBS_NPC_START + FC_NPC_TELE_MAGIC]);
        FAIL(err);
    }

    fc_destroy(&open);
}

/* ====================================================================== */
/* Test 7: Player ranged LOS gating                                        */
/* ====================================================================== */

static void test_player_attack_requires_los(void) {
    printf("\n=== Player Ranged LOS Gating ===\n");

    FcState state;
    char err[128];
    int actions[FC_NUM_ACTION_HEADS] = {0};

    init_manual_test_state(&state);
    state.walkable[11][10] = 0;  /* Full LOS blocker between player and target. */
    state.los_flags[11][10] = FC_LOS_FULL;

    /* A size-1 target makes this a single-ray blocking fixture. A single tile
     * cannot block every valid ray to Jad's 5x5 footprint. */
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state.npcs[0].movement_speed = 0;
    state.npcs[0].attack_timer = 999;
    state.npcs_remaining = 1;
    state.player.attack_target_idx = 0;
    state.player.approach_target = 1;
    state.player.attack_timer = 0;
    state.player.ammo_count = 50;

    fc_step(&state, actions);

    TEST("Player does not fire ranged attacks through blocked LOS");
    if (!state.attack_attempt_this_tick &&
        state.npcs[0].num_pending_hits == 0) PASS();
    else {
        snprintf(err, sizeof(err), "attack=%d pending_hits=%d",
                 state.attack_attempt_this_tick,
                 state.npcs[0].num_pending_hits);
        FAIL(err);
    }

    TEST("Auto-approach replans to a tile that has LOS");
    if (state.player.route_len > 0) {
        int ri = state.player.route_len - 1;
        int rx = state.player.route_x[ri];
        int ry = state.player.route_y[ri];
        int has_los = fc_has_los_between_areas(
            rx, ry, 1,
            state.npcs[0].x, state.npcs[0].y,
            state.npcs[0].size, state.los_flags);
        int rdist = fc_distance_to_npc(rx, ry, &state.npcs[0]);
        if (has_los && rdist <= state.player.weapon_range) PASS();
        else {
            snprintf(err, sizeof(err), "endpoint=(%d,%d) dist=%d los=%d", rx, ry, rdist, has_los);
            FAIL(err);
        }
    } else {
        snprintf(err, sizeof(err),
                 "no route planned; player=(%d,%d) route_idx=%d target=(%d,%d)",
                 state.player.x, state.player.y, state.player.route_idx,
                 state.npcs[0].x, state.npcs[0].y);
        FAIL(err);
    }

    fc_destroy(&state);
}

/* ====================================================================== */
/* Test 8: Yt-HurKot distraction on 0-damage player hits                   */
/* ====================================================================== */

static void test_hurkot_distraction(void) {
    printf("\n=== Yt-HurKot Distraction ===\n");

    FcState distract;
    init_manual_test_state(&distract);
    fc_npc_spawn(&distract.npcs[0], NPC_YT_HURKOT, 12, 10, 0);
    fc_queue_pending_hit(distract.npcs[0].pending_hits,
                         &distract.npcs[0].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         0, 1, ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&distract, 0);

    TEST("0-damage player hit still distracts Yt-HurKot");
    if (distract.npcs[0].healer_distracted == 1) PASS();
    else FAIL("healer_distracted did not flip");

    fc_destroy(&distract);
}

/* ====================================================================== */
/* Test 9: Jad healer respawn re-arm                                      */
/* ====================================================================== */

static int count_alive_hurkot(const FcState* state) {
    int count = 0;
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        const FcNpc* n = &state->npcs[i];
        if (n->active && !n->is_dead && n->npc_type == NPC_YT_HURKOT)
            count++;
    }
    return count;
}

static void remove_alive_hurkot(FcState* state) {
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* n = &state->npcs[i];
        if (n->active && n->npc_type == NPC_YT_HURKOT) {
            n->active = 0;
            n->is_dead = 1;
        }
    }
    state->npcs_remaining = 1;  /* Jad only */
}

static void test_jad_healer_respawn_requires_full_heal(void) {
    printf("\n=== Jad Healer Respawn Re-arm ===\n");

    FcState state;
    char err[128];
    int actions[FC_NUM_ACTION_HEADS] = {0};

    init_manual_test_state(&state);
    state.current_wave = FC_NUM_WAVES;
    state.npcs_remaining = 1;
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 20, 20, 0);

    int spawn_hp = FC_JAD_HEALER_THRESHOLD_HP_TENTHS;
    state.npcs[0].current_hp = spawn_hp - 10;
    fc_step(&state, actions);

    TEST("Jad first drop below 150 HP spawns four healers");
    int count = count_alive_hurkot(&state);
    if (count == FC_JAD_NUM_HEALERS && state.jad_healers_spawned) PASS();
    else {
        snprintf(err, sizeof(err), "healers=%d spawned=%d",
                 count, state.jad_healers_spawned);
        FAIL(err);
    }

    remove_alive_hurkot(&state);
    state.npcs[0].current_hp = spawn_hp + 10;
    fc_step(&state, actions);
    state.npcs[0].current_hp = spawn_hp - 10;
    fc_step(&state, actions);

    TEST("Healers do not respawn after Jad only heals above threshold");
    count = count_alive_hurkot(&state);
    if (count == 0 && state.jad_healers_spawned) PASS();
    else {
        snprintf(err, sizeof(err), "healers=%d spawned=%d",
                 count, state.jad_healers_spawned);
        FAIL(err);
    }

    state.npcs[0].current_hp = state.npcs[0].max_hp;
    fc_step(&state, actions);
    state.npcs[0].current_hp = spawn_hp - 10;
    fc_step(&state, actions);

    TEST("Healers respawn after Jad was restored to full HP");
    count = count_alive_hurkot(&state);
    if (count == FC_JAD_NUM_HEALERS && state.jad_healers_spawned) PASS();
    else {
        snprintf(err, sizeof(err), "healers=%d spawned=%d",
                 count, state.jad_healers_spawned);
        FAIL(err);
    }

    fc_destroy(&state);
}

/* ====================================================================== */
/* Test 10: Ready-idle prayer reward gating                               */
/* ====================================================================== */

static void test_ready_idle_prayer_gate(void) {
    printf("\n=== Ready-Idle Prayer Gate ===\n");

    char err[128];
    FcState state;
    FcRewardParams params = fc_reward_default_params();
    FcRewardRuntime runtime;
    FcRewardBreakdown b;

    init_manual_test_state(&state);
    fc_npc_spawn(&state.npcs[0], NPC_KET_ZEK, 12, 10, 0);
    state.player.hit_landed_this_tick = 1;
    state.player.hit_blocked_this_tick = 1;
    state.player.hit_locked_prayer_this_tick = PRAYER_PROTECT_MAGIC;
    state.player.hit_source_npc_type = NPC_KET_ZEK;
    state.correct_danger_prayer = 1;

    fc_reward_runtime_reset(&runtime);
    b = fc_reward_compute_breakdown(&state, &params, &runtime);

    TEST("Correct-prayer reward applies before ready-idle state");
    if (fabsf(b.correct_danger_prayer - params.w_correct_danger_prayer) < 0.0001f) PASS();
    else {
        snprintf(err, sizeof(err), "correct_danger=%.4f", b.correct_danger_prayer);
        FAIL(err);
    }

    runtime.ticks_since_attack = 1;
    b = fc_reward_compute_breakdown(&state, &params, &runtime);

    TEST("Correct-prayer reward is suppressed while ready-idle");
    if (fabsf(b.correct_danger_prayer) < 0.0001f) PASS();
    else {
        snprintf(err, sizeof(err), "correct_danger=%.4f", b.correct_danger_prayer);
        FAIL(err);
    }

    state.attack_attempt_this_tick = 1;
    b = fc_reward_compute_breakdown(&state, &params, &runtime);

    TEST("Attack attempt immediately re-enables correct-prayer reward");
    if (fabsf(b.correct_danger_prayer - params.w_correct_danger_prayer) < 0.0001f) PASS();
    else {
        snprintf(err, sizeof(err), "correct_danger=%.4f", b.correct_danger_prayer);
        FAIL(err);
    }

    fc_destroy(&state);
}

/* ====================================================================== */
/* Test 11: Jad prayer reward timing                                       */
/* ====================================================================== */

static void test_jad_prayer_reward_timing(void) {
    printf("\n=== Jad Prayer Reward Timing ===\n");

    char err[128];
    FcState state;
    FcRewardParams params = fc_reward_default_params();
    FcRewardRuntime runtime;
    FcRewardBreakdown b;
    FcPendingHit* queued;
    int reveal_tick;

    init_manual_test_state(&state);
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 12, 10, 0);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    fc_reward_runtime_reset(&runtime);
    reveal_tick = state.tick;

    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         97, 3, ATTACK_MAGIC, 0, 0);
    queued = &state.player.pending_hits[0];
    queued->prayer_snapshot = -1;
    queued->prayer_lock_tick = reveal_tick + 2;

    b = fc_reward_compute_breakdown(&state, &params, &runtime);
    TEST("Queued Jad attack gives no prayer reward before lock/resolve");
    if (fabsf(b.correct_jad_prayer) < 0.0001f &&
        fabsf(b.correct_danger_prayer) < 0.0001f) PASS();
    else {
        snprintf(err, sizeof(err), "jad=%.4f danger=%.4f",
                 b.correct_jad_prayer, b.correct_danger_prayer);
        FAIL(err);
    }

    clear_manual_tick_signals(&state);
    state.tick = reveal_tick;
    fc_resolve_player_pending_hits(&state);  /* reveal tick: 3 -> 2 */
    state.tick = reveal_tick + 1;
    fc_resolve_player_pending_hits(&state);  /* T+1: 2 -> 1 */
    state.tick = reveal_tick + 2;
    queued->prayer_snapshot = state.player.prayer;
    b = fc_reward_compute_breakdown(&state, &params, &runtime);
    TEST("Jad prayer lock tick still gives no reward before impact");
    if (queued->prayer_snapshot == PRAYER_PROTECT_MAGIC &&
        fabsf(b.correct_jad_prayer) < 0.0001f &&
        fabsf(b.correct_danger_prayer) < 0.0001f) PASS();
    else {
        snprintf(err, sizeof(err), "snap=%d jad=%.4f danger=%.4f",
                 queued->prayer_snapshot, b.correct_jad_prayer,
                 b.correct_danger_prayer);
        FAIL(err);
    }

    clear_manual_tick_signals(&state);
    state.tick = reveal_tick + 2;
    fc_resolve_player_pending_hits(&state);
    b = fc_reward_compute_breakdown(&state, &params, &runtime);
    TEST("Jad block receives shared correct-prayer reward on resolve tick");
    if (fabsf(b.raw[FC_RWD_CORRECT_JAD_PRAY] - 1.0f) < 0.0001f &&
        fabsf(b.raw[FC_RWD_CORRECT_DANGER_PRAY] - 1.0f) < 0.0001f &&
        fabsf(b.correct_jad_prayer) < 0.0001f &&
        fabsf(b.correct_danger_prayer - params.w_correct_danger_prayer) < 0.0001f &&
        state.player.hit_landed_this_tick == 1 &&
        state.player.hit_locked_prayer_this_tick == PRAYER_PROTECT_MAGIC) PASS();
    else {
        snprintf(err, sizeof(err), "jad=%.4f danger=%.4f landed=%d prayer=%d",
                 b.correct_jad_prayer, b.correct_danger_prayer,
                 state.player.hit_landed_this_tick,
                 state.player.hit_locked_prayer_this_tick);
        FAIL(err);
    }

    clear_manual_tick_signals(&state);
    state.tick = 3;
    state.correct_jad_prayer = 1;
    state.correct_danger_prayer = 1;
    state.player.hit_landed_this_tick = 1;
    state.player.hit_blocked_this_tick = 1;
    state.player.hit_locked_prayer_this_tick = PRAYER_PROTECT_MAGIC;
    state.player.hit_source_npc_type = NPC_TZTOK_JAD;
    runtime.ticks_since_attack = 1;
    b = fc_reward_compute_breakdown(&state, &params, &runtime);
    TEST("Ready-idle gate suppresses Jad shared prayer reward");
    if (fabsf(b.correct_jad_prayer) < 0.0001f &&
        fabsf(b.correct_danger_prayer) < 0.0001f) PASS();
    else {
        snprintf(err, sizeof(err), "jad=%.4f danger=%.4f",
                 b.correct_jad_prayer, b.correct_danger_prayer);
        FAIL(err);
    }

    fc_destroy(&state);
}

/* ====================================================================== */
/* Main                                                                    */
/* ====================================================================== */

int main(void) {
    printf("============================================================\n");
    printf("Fight Caves — Headless Backend Tests (Phase 9b)\n");
    printf("============================================================\n");
    printf("Action heads: %d  Obs size: %d  Mask size: %d\n",
           FC_NUM_ACTION_HEADS, FC_OBS_SIZE, FC_ACTION_MASK_SIZE);

    test_obs_normalized();
    test_reward_features();
    test_action_mask();
    test_determinism();
    test_multi_episode();
    test_melee_safespot_gating();
    test_player_attack_requires_los();
    test_hurkot_distraction();
    test_jad_healer_respawn_requires_full_heal();
    test_ready_idle_prayer_gate();
    test_jad_prayer_reward_timing();

    printf("\n============================================================\n");
    printf("RESULTS: %d/%d passed, %d failed\n", tests_passed, tests_run, tests_failed);
    printf("============================================================\n");

    return tests_failed > 0 ? 1 : 0;
}
