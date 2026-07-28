#include "fc_api.h"
#include "fc_combat.h"
#include "fc_contracts.h"
#include "fc_npc.h"
#include "fc_pathfinding.h"
#include "fc_reward.h"
#include "fc_wave.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char* msg) {
    printf("FAIL: %s\n", msg);
    return 1;
}

static int pass(const char* msg) {
    printf("PASS: %s\n", msg);
    return 0;
}

static const char* invalid_class_name(int idx) {
    switch (idx) {
        case FC_INVALID_ACTION_MOVE: return "move";
        case FC_INVALID_ACTION_ATTACK: return "attack";
        case FC_INVALID_ACTION_PRAYER: return "prayer";
        default: return "unknown";
    }
}

static void make_open_manual_state(FcState* state, int player_x, int player_y) {
    fc_init(state);
    fc_reset(state, 123);
    memset(state->npcs, 0, sizeof(state->npcs));
    memset(state->walkable, 1, sizeof(state->walkable));
    state->terminal = TERMINAL_NONE;
    state->current_wave = 1;
    state->npcs_remaining = 0;
    state->next_spawn_index = 0;
    state->player.x = player_x;
    state->player.y = player_y;
    state->player.attack_target_idx = -1;
    state->player.approach_target = 0;
    state->player.route_len = 0;
    state->player.route_idx = 0;
    state->player.attack_timer = 0;
    state->player.ammo_count = 1000;
    state->player.run_energy = 10000;
}

static int expected_npc_type_obs_offset(int npc_type) {
    switch (npc_type) {
        case NPC_TZ_KIH: return FC_NPC_TYPE_TZ_KIH;
        case NPC_TZ_KEK: return FC_NPC_TYPE_TZ_KEK;
        case NPC_TZ_KEK_SM: return FC_NPC_TYPE_TZ_KEK_SM;
        case NPC_TOK_XIL: return FC_NPC_TYPE_TOK_XIL;
        case NPC_YT_MEJKOT: return FC_NPC_TYPE_YT_MEJKOT;
        case NPC_KET_ZEK: return FC_NPC_TYPE_KET_ZEK;
        case NPC_TZTOK_JAD: return FC_NPC_TYPE_TZTOK_JAD;
        case NPC_YT_HURKOT: return FC_NPC_TYPE_YT_HURKOT;
        default: return -1;
    }
}

static int observation_slot_for_npc(const FcState* state, int npc_idx) {
    int active_indices[FC_VISIBLE_NPCS];
    int visible = fc_visible_npc_indices(state, active_indices);
    for (int slot = 0; slot < visible; slot++) {
        if (active_indices[slot] == npc_idx) return slot;
    }
    return -1;
}

static int check_invalid_case(const char* label,
                              FcState* state,
                              const int actions[FC_NUM_ACTION_HEADS],
                              int expected_class) {
    int direct[FC_INVALID_ACTION_CLASS_COUNT];
    int expected_any = (expected_class >= 0) ? 1 : 0;

    fc_action_invalid_classes(state, actions, direct);
    fc_step(state, actions);

    if (state->invalid_action_this_tick != expected_any) {
        char msg[160];
        snprintf(msg, sizeof(msg), "%s aggregate invalid=%d expected=%d",
                 label, state->invalid_action_this_tick, expected_any);
        return fail(msg);
    }

    for (int i = 0; i < FC_INVALID_ACTION_CLASS_COUNT; i++) {
        int expected = (i == expected_class) ? 1 : 0;
        if (direct[i] != expected) {
            char msg[192];
            snprintf(msg, sizeof(msg), "%s direct %s invalid=%d expected=%d",
                     label, invalid_class_name(i), direct[i], expected);
            return fail(msg);
        }
        if (state->invalid_action_class_this_tick[i] != expected) {
            char msg[192];
            snprintf(msg, sizeof(msg), "%s tick %s invalid=%d expected=%d",
                     label, invalid_class_name(i),
                     state->invalid_action_class_this_tick[i], expected);
            return fail(msg);
        }
        if (state->ep_invalid_action_classes[i] != expected) {
            char msg[192];
            snprintf(msg, sizeof(msg), "%s episode %s count=%d expected=%d",
                     label, invalid_class_name(i),
                     state->ep_invalid_action_classes[i], expected);
            return fail(msg);
        }
    }

    return 0;
}

static void configure_option_b_player(FcState* state, int weapon_range) {
    state->player.weapon_range = weapon_range;
    state->player.weapon_speed = 4;
    state->player.weapon_uses_ammo = 0;
    state->player.attack_timer = 0;
    state->player.attack_target_idx = -1;
    state->player.approach_target = 0;
}

static void spawn_static_guardrail_target(FcState* state, int x, int y) {
    fc_npc_spawn(&state->npcs[0], NPC_TOK_XIL, x, y, 0);
    state->npcs[0].movement_speed = 0;
    state->npcs[0].attack_timer = 999;
    state->npcs_remaining = 1;
}

static int guardrail_footprints_overlap(int ax, int ay, int asize,
                                        int bx, int by, int bsize) {
    return ax < bx + bsize && ax + asize > bx &&
           ay < by + bsize && ay + asize > by;
}

static int test_invalid_action_classes(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 0, 10);
    actions[0] = FC_MOVE_WALK_W;
    if (check_invalid_case("invalid_move", &state, actions, FC_INVALID_ACTION_MOVE)) {
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    memset(actions, 0, sizeof(actions));
    actions[1] = 1;
    if (check_invalid_case("invalid_attack", &state, actions, FC_INVALID_ACTION_ATTACK)) {
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    memset(actions, 0, sizeof(actions));
    actions[2] = FC_PRAYER_DIM;
    if (check_invalid_case("invalid_prayer", &state, actions, FC_INVALID_ACTION_PRAYER)) {
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    memset(actions, 0, sizeof(actions));
    actions[0] = FC_MOVE_WALK_N;
    actions[2] = FC_PRAYER_MAGIC;
    if (check_invalid_case("valid_nonzero", &state, actions, -1)) {
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    memset(actions, 0, sizeof(actions));
    actions[5] = FC_MOVE_TARGET_X_DIM + 1;
    actions[6] = FC_MOVE_TARGET_Y_DIM + 1;
    if (check_invalid_case("path_target_excluded", &state, actions, -1)) {
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);

    return pass("invalid-action diagnostics classify policy heads 0-2 without path-target noise");
}

static int test_hp_regeneration_interval(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    int initial_hp;

    if (FC_HP_REGEN_INTERVAL != 100) {
        return fail("HP regeneration interval is not 100 ticks");
    }

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, 50, 50, 0);
    state.npcs[0].movement_speed = 0;
    state.npcs[0].attack_timer = 10000;
    state.npcs_remaining = 1;
    state.player.current_hp = state.player.max_hp - 20;
    state.player.hp_regen_counter = 0;
    initial_hp = state.player.current_hp;

    for (int tick = 0; tick < FC_HP_REGEN_INTERVAL - 1; tick++) {
        fc_step(&state, actions);
    }
    if (state.player.current_hp != initial_hp) {
        fc_destroy(&state);
        return fail("HP regenerated before 100 ticks elapsed");
    }

    fc_step(&state, actions);
    if (state.player.current_hp != initial_hp + 10) {
        fc_destroy(&state);
        return fail("HP did not regenerate by exactly 1 HP on tick 100");
    }

    fc_destroy(&state);
    return pass("HP regenerates by 1 HP every 100 ticks");
}

static int test_target_identity(void) {
    FcState state;
    float obs[FC_OBS_SIZE];
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 5, 5);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 1, 1, 0);
    fc_npc_spawn(&state.npcs[1], NPC_TZTOK_JAD, 1, 10, 1);
    state.npcs_remaining = 2;

    fc_write_obs(&state, obs);
    int slot0_x = (int)lroundf(obs[FC_OBS_NPC_START + FC_NPC_X] * FC_ARENA_WIDTH);
    int slot0_y = (int)lroundf(obs[FC_OBS_NPC_START + FC_NPC_Y] * FC_ARENA_HEIGHT);
    if (slot0_x != 1 || slot0_y != 1) {
        char msg[128];
        snprintf(msg, sizeof(msg), "setup expected slot 0 to be Tz-Kih at (1,1), got (%d,%d)",
                 slot0_x, slot0_y);
        fc_destroy(&state);
        return fail(msg);
    }

    actions[0] = FC_MOVE_WALK_N;  /* movement is processed before attack target resolution */
    actions[1] = 1;               /* attack observed slot 0 */
    fc_step(&state, actions);

    if (state.attack_attempt_this_tick &&
        state.ep_attack_cycles_to_npc_type[NPC_TZ_KIH] == 1 &&
        state.ep_attack_cycles_to_npc_type[NPC_TZTOK_JAD] == 0) {
        fc_destroy(&state);
        return pass("attack slot stayed bound to the observed NPC identity");
    }

    char msg[160];
    snprintf(msg, sizeof(msg),
             "attack slot drifted or failed: Tz-Kih cycles=%d Jad cycles=%d target_idx=%d",
             state.ep_attack_cycles_to_npc_type[NPC_TZ_KIH],
             state.ep_attack_cycles_to_npc_type[NPC_TZTOK_JAD],
             state.player.attack_target_idx);
    fc_destroy(&state);
    return fail(msg);
}

static int test_npc_type_obs_one_hot(void) {
    const int npc_types[] = {
        NPC_TZ_KIH,
        NPC_TZ_KEK,
        NPC_TZ_KEK_SM,
        NPC_TOK_XIL,
        NPC_YT_MEJKOT,
        NPC_KET_ZEK,
        NPC_TZTOK_JAD,
        NPC_YT_HURKOT,
    };
    const char* npc_names[] = {
        "Tz-Kih",
        "Tz-Kek",
        "small Tz-Kek",
        "Tok-Xil",
        "Yt-MejKot",
        "Ket-Zek",
        "TzTok-Jad",
        "Yt-HurKot",
    };

    for (int i = 0; i < 8; i++) {
        FcState state;
        float obs[FC_OBS_SIZE];
        int expected_offset = expected_npc_type_obs_offset(npc_types[i]);
        int base = FC_OBS_NPC_START;

        make_open_manual_state(&state, 10, 10);
        fc_npc_spawn(&state.npcs[0], npc_types[i], 12, 10, 0);
        state.npcs_remaining = 1;
        fc_write_obs(&state, obs);

        if (obs[base + FC_NPC_VALID] < 0.5f) {
            char msg[128];
            snprintf(msg, sizeof(msg), "%s slot not marked valid", npc_names[i]);
            fc_destroy(&state);
            return fail(msg);
        }

        int type_bits_on = 0;
        for (int offset = FC_NPC_TYPE_TZ_KIH; offset <= FC_NPC_TYPE_YT_HURKOT; offset++) {
            int is_on = obs[base + offset] > 0.5f;
            int should_be_on = offset == expected_offset;
            if (is_on != should_be_on) {
                char msg[192];
                snprintf(msg, sizeof(msg),
                         "%s type bit offset %d was %d, expected %d",
                         npc_names[i], offset, is_on, should_be_on);
                fc_destroy(&state);
                return fail(msg);
            }
            type_bits_on += is_on;
        }

        if (type_bits_on != 1) {
            char msg[128];
            snprintf(msg, sizeof(msg), "%s had %d type bits set",
                     npc_names[i], type_bits_on);
            fc_destroy(&state);
            return fail(msg);
        }

        int empty_base = FC_OBS_NPC_START + FC_OBS_NPC_STRIDE;
        for (int offset = FC_NPC_TYPE_TZ_KIH; offset <= FC_NPC_TYPE_YT_HURKOT; offset++) {
            if (obs[empty_base + offset] > 0.5f) {
                char msg[160];
                snprintf(msg, sizeof(msg),
                         "%s leaked type bit %d into empty slot",
                         npc_names[i], offset);
                fc_destroy(&state);
                return fail(msg);
            }
        }

        fc_destroy(&state);
    }

    return pass("visible NPC slots expose exactly one NPC type bit");
}

static int test_zero_damage_reward(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state.npcs_remaining = 1;
    fc_queue_pending_hit(state.npcs[0].pending_hits,
                         &state.npcs[0].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         0, 1, ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, 0);

    memset(&params, 0, sizeof(params));
    params.w_damage_dealt = 1.0f;
    fc_reward_runtime_reset(&runtime);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);

    if (state.hits_landed_this_tick == 0 &&
        state.ep_resolved_hits_to_npc_type[NPC_TZ_KIH] == 1 &&
        state.ep_damaging_hits_to_npc_type[NPC_TZ_KIH] == 0 &&
        state.ep_damage_to_npc_type[NPC_TZ_KIH] == 0 &&
        fabsf(breakdown.damage_dealt) < 0.0001f) {
        fc_destroy(&state);
        return pass("zero-damage hits are logged as resolved hits without damage reward");
    }

    char msg[160];
    snprintf(msg, sizeof(msg),
             "zero-damage hit metrics wrong: hits=%d resolved=%d damaging=%d damage=%d reward=%.4f",
             state.hits_landed_this_tick,
             state.ep_resolved_hits_to_npc_type[NPC_TZ_KIH],
             state.ep_damaging_hits_to_npc_type[NPC_TZ_KIH],
             state.ep_damage_to_npc_type[NPC_TZ_KIH],
             breakdown.damage_dealt);
    fc_destroy(&state);
    return fail(msg);
}

static int test_safespot_reward_disabled(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 20, 10, 0);
    state.npcs_remaining = 1;
    state.safespot_attack_this_tick = 1;

    memset(&params, 0, sizeof(params));
    fc_reward_runtime_reset(&runtime);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);

    if (fabsf(breakdown.total) < 0.0001f) {
        fc_destroy(&state);
        return pass("direct safespot reward is disabled while LOS mechanics remain intact");
    }

    char msg[160];
    snprintf(msg, sizeof(msg), "safespot reward still affects total: total=%.4f",
             breakdown.total);
    fc_destroy(&state);
    return fail(msg);
}

static int test_npc_heal_penalty_actual_heal(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_MEJKOT, 11, 11, 0);
    fc_npc_spawn(&state.npcs[1], NPC_TZ_KIH, 15, 11, 1);
    state.npcs_remaining = 2;
    state.npcs[0].attack_timer = 0;
    state.npcs[0].heal_timer = 0;
    state.npcs[1].current_hp = state.npcs[1].max_hp / 4;

    int before = state.npcs[1].current_hp;
    fc_npc_tick(&state, 0);
    int restored = state.npcs[1].current_hp - before;

    if (restored <= 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "expected Yt-MejKot to restore HP, restored=%d",
                 restored);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.npc_heal_procs_this_tick != 1 ||
        state.npc_heal_amount_this_tick != restored) {
        char msg[160];
        snprintf(msg, sizeof(msg), "heal tracking wrong: procs=%d amount=%d restored=%d",
                 state.npc_heal_procs_this_tick,
                 state.npc_heal_amount_this_tick,
                 restored);
        fc_destroy(&state);
        return fail(msg);
    }

    memset(&params, 0, sizeof(params));
    params.shape_npc_heal_penalty = -0.3f;
    fc_reward_runtime_reset(&runtime);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);

    if (fabsf(breakdown.npc_heal + 0.3f) < 0.0001f &&
        fabsf(breakdown.total + 0.3f) < 0.0001f) {
        fc_destroy(&state);
        return pass("actual NPC healing triggers one general heal penalty");
    }

    char msg[160];
    snprintf(msg, sizeof(msg), "heal reward wrong: npc_heal=%.4f total=%.4f",
             breakdown.npc_heal, breakdown.total);
    fc_destroy(&state);
    return fail(msg);
}

static int test_prayer_loss_penalty(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    params = fc_reward_default_params();
    params.w_progress = 0.0f;
    params.w_tick_penalty = 0.0f;

    fc_reward_runtime_reset(&runtime);
    state.prayer_lost_this_tick = 10;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(breakdown.raw[FC_RWD_PRAYER_LOST] - 1.0f) > 0.0001f ||
        fabsf(breakdown.prayer_lost + 0.02f) > 0.0001f ||
        fabsf(breakdown.total + 0.02f) > 0.0001f) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "1pt prayer loss wrong: raw=%.4f prayer_lost=%.4f total=%.4f",
                 breakdown.raw[FC_RWD_PRAYER_LOST],
                 breakdown.prayer_lost,
                 breakdown.total);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_reward_runtime_reset(&runtime);
    state.prayer_lost_this_tick = 30;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(breakdown.raw[FC_RWD_PRAYER_LOST] - 3.0f) < 0.0001f &&
        fabsf(breakdown.prayer_lost + 0.06f) < 0.0001f &&
        fabsf(breakdown.total + 0.06f) < 0.0001f) {
        fc_destroy(&state);
        return pass("prayer loss penalty scales per prayer point lost");
    }

    char msg[192];
    snprintf(msg, sizeof(msg),
             "3pt prayer loss wrong: raw=%.4f prayer_lost=%.4f total=%.4f",
             breakdown.raw[FC_RWD_PRAYER_LOST],
             breakdown.prayer_lost,
             breakdown.total);
    fc_destroy(&state);
    return fail(msg);
}

static int test_correct_prayer_reward_all_npcs(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 12, 10, 0);
    state.npcs_remaining = 1;
    state.player.prayer = PRAYER_PROTECT_MAGIC;

    if (!fc_queue_pending_hit(state.player.pending_hits,
                              &state.player.num_pending_hits,
                              FC_MAX_PENDING_HITS,
                              500, 1, ATTACK_MAGIC, 0, 0)) {
        fc_destroy(&state);
        return fail("could not queue Jad hit for shared prayer reward test");
    }
    state.player.pending_hits[0].prayer_snapshot = PRAYER_PROTECT_MAGIC;
    fc_resolve_player_pending_hits(&state);

    if (!state.correct_jad_prayer || !state.correct_danger_prayer ||
        state.wrong_jad_prayer || state.wrong_danger_prayer ||
        state.damage_taken_this_tick != 0) {
        char msg[224];
        snprintf(msg, sizeof(msg),
                 "Jad block flags wrong: jad=%d danger=%d wrong_jad=%d wrong_danger=%d damage=%d",
                 state.correct_jad_prayer, state.correct_danger_prayer,
                 state.wrong_jad_prayer, state.wrong_danger_prayer,
                 state.damage_taken_this_tick);
        fc_destroy(&state);
        return fail(msg);
    }

    memset(&params, 0, sizeof(params));
    params.w_correct_danger_prayer = 0.005f;
    params.w_correct_jad_prayer = 0.0f;
    fc_reward_runtime_reset(&runtime);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);

    if (fabsf(breakdown.raw[FC_RWD_CORRECT_JAD_PRAY] - 1.0f) > 0.0001f ||
        fabsf(breakdown.raw[FC_RWD_CORRECT_DANGER_PRAY] - 1.0f) > 0.0001f ||
        fabsf(breakdown.correct_jad_prayer) > 0.0001f ||
        fabsf(breakdown.correct_danger_prayer - 0.005f) > 0.0001f ||
        fabsf(breakdown.total - 0.005f) > 0.0001f) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Jad shared reward wrong: raw_jad=%.3f raw_all=%.3f jad=%.4f all=%.4f total=%.4f",
                 breakdown.raw[FC_RWD_CORRECT_JAD_PRAY],
                 breakdown.raw[FC_RWD_CORRECT_DANGER_PRAY],
                 breakdown.correct_jad_prayer,
                 breakdown.correct_danger_prayer,
                 breakdown.total);
        fc_destroy(&state);
        return fail(msg);
    }

    runtime.ticks_since_attack = 100;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(breakdown.correct_jad_prayer) > 0.0001f ||
        fabsf(breakdown.correct_danger_prayer) > 0.0001f) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "attack-idle gate changed: jad=%.4f all=%.4f",
                 breakdown.correct_jad_prayer,
                 breakdown.correct_danger_prayer);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("Jad uses shared correct-prayer reward with existing idle gate preserved");
}

static int test_no_attack_penalty_wave_scaled(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;
    float expected;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, 12, 10, 0);
    state.npcs_remaining = 1;
    state.current_wave = 25;
    state.player.attack_timer = 0;

    params = fc_reward_default_params();
    params.w_progress = 0.0f;
    params.w_tick_penalty = 0.0f;
    params.shape_no_progress_start_1 = 0;
    params.shape_no_progress_start_2 = 0;
    params.shape_no_progress_start_3 = 0;

    fc_reward_runtime_begin_episode(&runtime, &state);
    runtime.ticks_since_attack = params.shape_no_attack_start - 1;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (breakdown.no_attack != 0.0f) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "no-attack penalty fired too early: timer=%d no_attack=%.6f",
                 runtime.ticks_since_attack, breakdown.no_attack);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_reward_runtime_begin_episode(&runtime, &state);
    runtime.ticks_since_attack = params.shape_no_attack_start;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    expected = params.shape_no_attack_base_penalty *
        (1.0f + params.shape_no_attack_wave_scale *
         ((float)state.current_wave - 1.0f));
    if (fabsf(breakdown.no_attack - expected) < 0.0001f &&
        fabsf(breakdown.total - expected) < 0.0001f) {
        fc_destroy(&state);
        return pass("no-attack penalty starts after threshold and scales by wave");
    }

    char msg[192];
    snprintf(msg, sizeof(msg),
             "wave-scaled no-attack wrong: timer=%d no_attack=%.6f expected=%.6f total=%.6f",
             runtime.ticks_since_attack, breakdown.no_attack, expected,
             breakdown.total);
    fc_destroy(&state);
    return fail(msg);
}

static int test_player_death_progress_scaled(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;
    const float progress = (53.0f + 0.5f) / (float)FC_NUM_WAVES;
    const float expected_scaled = -(0.1f + 0.9f * progress);

    memset(&params, 0, sizeof(params));
    params.w_player_death = -1.0f;
    params.scale_player_death_with_progress = 1;
    params.player_death_min_scale = 0.1f;

    if (fabsf(fc_reward_player_death_scale(&params, 0.0f) - 0.1f) > 0.0001f ||
        fabsf(fc_reward_player_death_scale(&params, 0.5f) - 0.55f) > 0.0001f ||
        fabsf(fc_reward_player_death_scale(&params, 1.0f) - 1.0f) > 0.0001f) {
        return fail("player-death scale does not follow the configured linear schedule");
    }

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_MEJKOT, 12, 10, 0);
    state.npcs[0].current_hp = 400;
    state.npcs_remaining = 1;
    state.current_wave = 54;
    state.terminal = TERMINAL_PLAYER_DEATH;

    memset(&runtime, 0, sizeof(runtime));
    runtime.required_work_at_wave_start = 800.0f;
    runtime.cave_progress_prev = 53.0f / (float)FC_NUM_WAVES;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(runtime.last_cave_progress - progress) > 0.0001f ||
        fabsf(breakdown.player_death - expected_scaled) > 0.0001f ||
        fabsf(breakdown.total - expected_scaled) > 0.0001f) {
        char msg[224];
        snprintf(msg, sizeof(msg),
                 "scaled death wrong: progress=%.6f death=%.6f total=%.6f expected=%.6f",
                 runtime.last_cave_progress, breakdown.player_death,
                 breakdown.total, expected_scaled);
        fc_destroy(&state);
        return fail(msg);
    }

    params.scale_player_death_with_progress = 0;
    memset(&runtime, 0, sizeof(runtime));
    runtime.required_work_at_wave_start = 800.0f;
    runtime.cave_progress_prev = 53.0f / (float)FC_NUM_WAVES;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(breakdown.player_death + 1.0f) > 0.0001f ||
        fabsf(breakdown.total + 1.0f) > 0.0001f) {
        fc_destroy(&state);
        return fail("disabled progress scaling no longer preserves fixed death reward");
    }

    fc_destroy(&state);
    return pass("player death scales linearly with cave progress and remains opt-in");
}

static int test_simple_reward_zeroed_channels(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;
    float channels[FC_CH_COUNT];

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 30, 30, 0);
    state.npcs_remaining = 1;
    state.player.prayer = FC_PRAYER_MELEE;

    /* Populate the raw events behind every zero-weight scalar channel. */
    state.damage_dealt_this_tick = 100;
    state.hits_landed_this_tick = 1;
    state.damage_taken_this_tick = 100;
    state.npcs_killed_this_tick = 1;
    state.wave_just_cleared = 1;
    state.jad_killed = 1;
    state.correct_jad_prayer = 1;
    state.correct_danger_prayer = 1;
    state.prayer_lost_this_tick = 10;
    state.invalid_action_this_tick = 1;
    state.jad_heal_procs_this_tick = 1;
    state.npc_heal_procs_this_tick = 1;

    memset(&params, 0, sizeof(params));
    params.w_progress = 0.0001f;
    params.negative_progress_multiplier = 1.1f;
    params.w_cave_complete = 1.0f;
    params.w_player_death = -1.0f;
    params.scale_player_death_with_progress = 1;
    params.player_death_min_scale = 0.1f;
    params.shape_no_progress_start_1 = 800;
    params.shape_no_progress_start_2 = 1600;
    params.shape_no_progress_start_3 = 2400;
    params.shape_no_attack_start = 50;
    params.shape_no_attack_wave_scale = 0.05f;

    for (int scenario = 0; scenario < 2; scenario++) {
        fc_reward_runtime_begin_episode(&runtime, &state);
        runtime.ticks_since_positive_progress = 3000;
        runtime.ticks_since_attack = scenario == 0 ? 0 : 100;
        runtime.ticks_in_wave = 3000;
        state.attack_attempt_this_tick = scenario == 0 ? 1 : 0;
        breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
        fc_reward_breakdown_channels(&breakdown, channels);

        for (int i = 0; i < FC_CH_COUNT; i++) {
            if (fabsf(channels[i]) > 0.000001f) {
                char msg[224];
                snprintf(msg, sizeof(msg),
                         "zeroed simple-reward channel %s fired in scenario %d with value %.6f",
                         FC_CH_NAMES[i], scenario, channels[i]);
                fc_destroy(&state);
                return fail(msg);
            }
        }
        if (fabsf(breakdown.total) > 0.000001f) {
            char msg[160];
            snprintf(msg, sizeof(msg),
                     "zeroed simple-reward channels changed scenario %d total by %.6f",
                     scenario, breakdown.total);
            fc_destroy(&state);
            return fail(msg);
        }
    }

    fc_destroy(&state);
    return pass("zeroed simple-reward channels cannot affect scalar reward");
}

static int test_npc_kill_reward_eligibility(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;
    float reward_features[FC_REWARD_FEATURES];
    float obs[FC_TOTAL_OBS];
    int actions[FC_NUM_ACTION_HEADS] = {0};

    memset(&params, 0, sizeof(params));
    params.w_npc_kill = 0.25f;

    /* An ordinary death remains one factual kill and one rewarded kill. */
    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state.npcs_remaining = 1;
    fc_reward_runtime_begin_episode(&runtime, &state);
    fc_write_obs(&state, obs);
    if (obs[FC_OBS_NPC_START + FC_NPC_VALID] != 1.0f ||
        obs[FC_OBS_NPC_START + FC_NPC_HP] <= 0.0f ||
        obs[FC_OBS_NPC_START + FC_NPC_KILL_REWARD_ELIGIBLE] != 1.0f) {
        fc_destroy(&state);
        return fail("living ordinary NPC is not visibly kill-reward eligible");
    }

    fc_queue_pending_hit(state.npcs[0].pending_hits,
                         &state.npcs[0].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         state.npcs[0].current_hp, 1,
                         ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, 0);
    fc_write_reward_features(&state, reward_features);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    fc_write_obs(&state, obs);
    if (state.npcs_killed_this_tick != 1 ||
        state.respawned_jad_healers_killed_this_tick != 0 ||
        state.total_npcs_killed != 1 ||
        reward_features[FC_RWD_NPC_KILL] != 1.0f ||
        fabsf(breakdown.npc_kill - 0.25f) > 0.0001f ||
        obs[FC_OBS_NPC_START + FC_NPC_VALID] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_REMAINING] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_REWARDABLE_NPC_KILL] != 1.0f) {
        fc_destroy(&state);
        return fail("ordinary NPC death is not counted, rewarded, or observable correctly");
    }
    fc_step(&state, actions);
    fc_write_obs(&state, obs);
    if (obs[FC_OBS_META_START + FC_OBS_META_REWARDABLE_NPC_KILL] != 0.0f) {
        fc_destroy(&state);
        return fail("rewardable NPC-kill observation persisted beyond one tick");
    }
    fc_destroy(&state);

    /* First-generation Jad healers pay normally; later generations do not. */
    make_open_manual_state(&state, 20, 20);
    state.current_wave = FC_NUM_WAVES;
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 10, 10, 0);
    state.npcs[0].current_hp = FC_JAD_HEALER_THRESHOLD_HP_TENTHS - 10;
    state.npcs[0].attack_timer = 999;
    state.npcs[0].movement_speed = 0;
    state.npcs_remaining = 1;
    state.next_spawn_index = 1;
    fc_step(&state, actions);

    int first_healer_idx = -1;
    int first_generation_count = 0;
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* npc = &state.npcs[i];
        if (!npc->active || npc->is_dead || npc->npc_type != NPC_YT_HURKOT) continue;
        first_generation_count++;
        if (npc->is_respawned_jad_healer) {
            fc_destroy(&state);
            return fail("first Jad-healer generation was marked as respawned");
        }
        if (first_healer_idx < 0) first_healer_idx = i;
    }
    if (first_generation_count != FC_JAD_NUM_HEALERS ||
        state.jad_healer_spawn_generations != 1 || first_healer_idx < 0) {
        fc_destroy(&state);
        return fail("initial Jad-healer generation did not spawn completely");
    }

    fc_reward_runtime_begin_episode(&runtime, &state);
    FcNpc* first_healer = &state.npcs[first_healer_idx];
    fc_write_obs(&state, obs);
    int first_healer_slot = observation_slot_for_npc(&state, first_healer_idx);
    if (first_healer_slot < 0 ||
        obs[FC_OBS_NPC_START + first_healer_slot * FC_OBS_NPC_STRIDE +
            FC_NPC_KILL_REWARD_ELIGIBLE] != 1.0f) {
        fc_destroy(&state);
        return fail("first-generation Jad healer is not visibly kill-reward eligible");
    }
    fc_queue_pending_hit(first_healer->pending_hits,
                         &first_healer->num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         first_healer->current_hp, 1,
                         ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, first_healer_idx);
    fc_write_reward_features(&state, reward_features);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    fc_write_obs(&state, obs);
    if (reward_features[FC_RWD_NPC_KILL] != 1.0f ||
        fabsf(breakdown.npc_kill - 0.25f) > 0.0001f ||
        obs[FC_OBS_META_START + FC_OBS_META_REWARDABLE_NPC_KILL] != 1.0f) {
        fc_destroy(&state);
        return fail("first-generation Jad healer did not grant the normal kill reward");
    }

    /* Free the dead slot, re-arm at full HP, then cross the threshold again. */
    first_healer->active = 0;
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        if (state.npcs[i].active && state.npcs[i].npc_type == NPC_YT_HURKOT) {
            state.npcs[i].movement_speed = 0;
            state.npcs[i].heal_timer = 999;
        }
    }
    state.npcs[0].current_hp = state.npcs[0].max_hp;
    state.npcs[0].attack_timer = 999;
    fc_step(&state, actions);
    if (state.jad_healers_spawned != 0) {
        fc_destroy(&state);
        return fail("Jad reaching full HP did not re-arm healer spawning");
    }

    state.npcs[0].current_hp = FC_JAD_HEALER_THRESHOLD_HP_TENTHS - 10;
    state.npcs[0].attack_timer = 999;
    fc_step(&state, actions);

    int respawned_healer_idx = -1;
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* npc = &state.npcs[i];
        if (npc->active && !npc->is_dead &&
            npc->npc_type == NPC_YT_HURKOT && npc->is_respawned_jad_healer) {
            respawned_healer_idx = i;
            break;
        }
    }
    if (state.jad_healer_spawn_generations != 2 || respawned_healer_idx < 0) {
        fc_destroy(&state);
        return fail("later Jad-healer generation was not tagged as respawned");
    }

    FcNpc* respawned_healer = &state.npcs[respawned_healer_idx];
    fc_write_obs(&state, obs);
    int respawned_healer_slot = observation_slot_for_npc(&state, respawned_healer_idx);
    if (respawned_healer_slot < 0 ||
        obs[FC_OBS_NPC_START + respawned_healer_slot * FC_OBS_NPC_STRIDE +
            FC_NPC_KILL_REWARD_ELIGIBLE] != 0.0f) {
        fc_destroy(&state);
        return fail("respawned Jad healer is not visibly excluded from kill reward");
    }
    fc_queue_pending_hit(respawned_healer->pending_hits,
                         &respawned_healer->num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         respawned_healer->current_hp, 1,
                         ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, respawned_healer_idx);
    fc_write_reward_features(&state, reward_features);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    fc_write_obs(&state, obs);
    if (state.npcs_killed_this_tick != 1 ||
        state.respawned_jad_healers_killed_this_tick != 1 ||
        state.total_npcs_killed != 2 ||
        reward_features[FC_RWD_NPC_KILL] != 0.0f ||
        fabsf(breakdown.npc_kill) > 0.0001f ||
        obs[FC_OBS_META_START + FC_OBS_META_REWARDABLE_NPC_KILL] != 0.0f) {
        fc_destroy(&state);
        return fail("respawned Jad healer affected reward or disappeared from kill analytics");
    }

    fc_destroy(&state);
    return pass("NPC kills pay once, while respawned Jad healers remain analytics-only");
}

static int test_tz_kek_split_kill_rewards(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;
    float reward_features[FC_REWARD_FEATURES];
    float obs[FC_TOTAL_OBS];
    int child_indices[2] = {-1, -1};
    int child_count = 0;

    make_open_manual_state(&state, 8, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KEK, 12, 10, 0);
    state.npcs_remaining = 2;
    state.next_spawn_index = 1;

    memset(&params, 0, sizeof(params));
    params.w_npc_kill = 0.25f;
    fc_reward_runtime_begin_episode(&runtime, &state);

    fc_write_obs(&state, obs);
    if (obs[FC_OBS_NPC_START + FC_NPC_TYPE_TZ_KEK] != 1.0f ||
        obs[FC_OBS_NPC_START + FC_NPC_KILL_REWARD_ELIGIBLE] != 1.0f) {
        fc_destroy(&state);
        return fail("parent Tz-Kek is not visibly kill-reward eligible");
    }

    fc_queue_pending_hit(state.npcs[0].pending_hits,
                         &state.npcs[0].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         state.npcs[0].current_hp, 1,
                         ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, 0);
    fc_write_reward_features(&state, reward_features);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    fc_write_obs(&state, obs);

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        if (state.npcs[i].active && !state.npcs[i].is_dead &&
            state.npcs[i].npc_type == NPC_TZ_KEK_SM) {
            if (child_count < 2) child_indices[child_count] = i;
            child_count++;
        }
    }
    if (state.npcs_killed_this_tick != 1 ||
        state.total_npcs_killed != 1 ||
        state.npcs_remaining != 2 ||
        reward_features[FC_RWD_NPC_KILL] != 1.0f ||
        fabsf(breakdown.npc_kill - 0.25f) > 0.0001f ||
        obs[FC_OBS_META_START + FC_OBS_META_REWARDABLE_NPC_KILL] != 1.0f ||
        child_count != 2) {
        fc_destroy(&state);
        return fail("parent Tz-Kek death did not pay once and split into two children");
    }

    for (int child = 0; child < 2; child++) {
        int slot = observation_slot_for_npc(&state, child_indices[child]);
        if (slot < 0) {
            fc_destroy(&state);
            return fail("split Tz-Kek child is missing from visible NPC observations");
        }
        int base = FC_OBS_NPC_START + slot * FC_OBS_NPC_STRIDE;
        if (obs[base + FC_NPC_TYPE_TZ_KEK_SM] != 1.0f ||
            obs[base + FC_NPC_KILL_REWARD_ELIGIBLE] != 1.0f) {
            fc_destroy(&state);
            return fail("split Tz-Kek child is not visibly kill-reward eligible");
        }
    }

    for (int child = 0; child < 2; child++) {
        state.npcs_killed_this_tick = 0;
        state.respawned_jad_healers_killed_this_tick = 0;
        FcNpc* npc = &state.npcs[child_indices[child]];
        fc_queue_pending_hit(npc->pending_hits,
                             &npc->num_pending_hits,
                             FC_MAX_PENDING_HITS,
                             npc->current_hp, 1,
                             ATTACK_RANGED, -1, 0);
        fc_resolve_npc_pending_hits(&state, child_indices[child]);
        fc_write_reward_features(&state, reward_features);
        breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
        fc_write_obs(&state, obs);

        if (state.npcs_killed_this_tick != 1 ||
            state.respawned_jad_healers_killed_this_tick != 0 ||
            state.total_npcs_killed != child + 2 ||
            state.npcs_remaining != 1 - child ||
            reward_features[FC_RWD_NPC_KILL] != 1.0f ||
            fabsf(breakdown.npc_kill - 0.25f) > 0.0001f ||
            obs[FC_OBS_META_START + FC_OBS_META_REWARDABLE_NPC_KILL] != 1.0f) {
            fc_destroy(&state);
            return fail("a split Tz-Kek child did not pay the same single kill reward");
        }
    }

    fc_destroy(&state);
    return pass("parent Tz-Kek and both split children each pay one equal kill reward");
}

static int test_wave_clear_reward_scaling(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;
    float obs[FC_TOTAL_OBS];
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    state.current_wave = 15;
    state.npcs_remaining = 0;
    fc_reward_runtime_begin_episode(&runtime, &state);

    if (!fc_wave_check_advance(&state) || state.current_wave != 16 ||
        !state.wave_just_cleared) {
        fc_destroy(&state);
        return fail("wave 15 did not emit one wave-clear transition");
    }

    memset(&params, 0, sizeof(params));
    params.w_wave_clear = 0.002f;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    fc_write_obs(&state, obs);
    if (fabsf(breakdown.wave_clear - 0.030f) > 0.0001f ||
        fabsf(breakdown.total - 0.030f) > 0.0001f ||
        obs[FC_OBS_META_START + FC_OBS_META_WAVE_CLR] != 1.0f ||
        fabsf(obs[FC_OBS_META_START + FC_OBS_META_WAVE] -
              (16.0f / (float)FC_NUM_WAVES)) > 0.0001f) {
        fc_destroy(&state);
        return fail("wave-clear reward or its policy observations are incorrect");
    }

    fc_step(&state, actions);
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (state.wave_just_cleared || fabsf(breakdown.wave_clear) > 0.0001f) {
        fc_destroy(&state);
        return fail("wave-clear reward persisted beyond its transition tick");
    }

    fc_destroy(&state);
    return pass("wave-clear reward scales by cleared wave and fires for one transition");
}

static int test_net_progress_required_work(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;
    float positive_progress;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state.npcs_remaining = 1;

    memset(&params, 0, sizeof(params));
    params.w_progress = 0.001f;
    params.negative_progress_multiplier = 1.1f;
    fc_reward_runtime_begin_episode(&runtime, &state);

    if (fabsf(runtime.required_work_at_wave_start - 100.0f) > 0.0001f) {
        char msg[128];
        snprintf(msg, sizeof(msg), "initial work %.4f expected 100",
                 runtime.required_work_at_wave_start);
        fc_destroy(&state);
        return fail(msg);
    }

    state.npcs[0].current_hp = 90;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    positive_progress = breakdown.progress;
    if (fabsf(runtime.last_required_work_remaining - 90.0f) > 0.0001f ||
        fabsf(runtime.last_net_required_work_removed - 10.0f) > 0.0001f ||
        fabsf(breakdown.progress - 0.01f) > 0.0001f) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "damage progress wrong: work=%.4f net=%.4f progress=%.6f",
                 runtime.last_required_work_remaining,
                 runtime.last_net_required_work_removed,
                 breakdown.progress);
        fc_destroy(&state);
        return fail(msg);
    }

    state.npcs[0].current_hp = 100;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(runtime.last_required_work_remaining - 100.0f) > 0.0001f ||
        fabsf(runtime.last_net_required_work_removed + 10.0f) > 0.0001f ||
        fabsf(breakdown.progress + 0.011f) > 0.0001f ||
        positive_progress + breakdown.progress >= -0.0001f ||
        runtime.ticks_since_positive_progress <= 0) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "healed progress wrong: work=%.4f net=%.4f progress=%.6f timer=%d",
                 runtime.last_required_work_remaining,
                 runtime.last_net_required_work_removed,
                 breakdown.progress,
                 runtime.ticks_since_positive_progress);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("negative progress is 10% stronger while positive progress is unchanged");
}

static int test_net_progress_wave_clear_transition(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state.npcs_remaining = 1;

    memset(&params, 0, sizeof(params));
    params.w_progress = 0.001f;
    params.negative_progress_multiplier = 1.0f;
    fc_reward_runtime_begin_episode(&runtime, &state);

    state.npcs[0].is_dead = 1;
    state.npcs[0].active = 0;
    state.npcs_remaining = 0;
    fc_wave_check_advance(&state);

    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (!state.wave_just_cleared ||
        state.current_wave != 2 ||
        fabsf(runtime.last_net_required_work_removed - 100.0f) > 0.0001f ||
        fabsf(breakdown.progress - 0.1f) > 0.0001f ||
        runtime.last_required_work_remaining <= 100.0f) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "wave clear progress wrong wave=%d net=%.4f progress=%.6f next_work=%.4f",
                 state.current_wave,
                 runtime.last_net_required_work_removed,
                 breakdown.progress,
                 runtime.last_required_work_remaining);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("net progress pays old-wave work without penalizing next-wave spawn");
}

static int test_net_progress_tz_kek_accounting(void) {
    FcState state;
    FcRewardRuntime runtime;
    float work;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KEK, 12, 10, 0);
    state.npcs_remaining = 2;
    fc_reward_runtime_begin_episode(&runtime, &state);

    if (fabsf(runtime.required_work_at_wave_start - 400.0f) > 0.0001f) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Tz-Kek start work %.4f expected 400",
                 runtime.required_work_at_wave_start);
        fc_destroy(&state);
        return fail(msg);
    }

    state.npcs[0].is_dead = 1;
    fc_npc_tz_kek_split(&state, state.npcs[0].x, state.npcs[0].y);
    work = fc_reward_required_work_remaining(&state);

    if (fabsf(work - 200.0f) < 0.0001f) {
        fc_destroy(&state);
        return pass("Tz-Kek parent death leaves latent small-Kek work");
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "Tz-Kek split work %.4f expected 200", work);
    fc_destroy(&state);
    return fail(msg);
}

static int test_net_progress_jad_accounting(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 20, 20);
    state.current_wave = FC_NUM_WAVES;
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 10, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_YT_HURKOT, 16, 10, 1);
    state.npcs_remaining = 2;

    memset(&params, 0, sizeof(params));
    params.w_progress = 0.001f;
    params.negative_progress_multiplier = 1.0f;
    fc_reward_runtime_begin_episode(&runtime, &state);

    if (fabsf(runtime.required_work_at_wave_start - 2500.0f) > 0.0001f) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Jad start work %.4f expected 2500",
                 runtime.required_work_at_wave_start);
        fc_destroy(&state);
        return fail(msg);
    }

    state.npcs[0].current_hp = 2400;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(runtime.last_net_required_work_removed - 100.0f) > 0.01f ||
        fabsf(breakdown.progress - 0.1f) > 0.0001f) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "Jad damage progress wrong net=%.4f progress=%.6f",
                 runtime.last_net_required_work_removed,
                 breakdown.progress);
        fc_destroy(&state);
        return fail(msg);
    }

    state.npcs[0].current_hp = 2450;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (fabsf(runtime.last_net_required_work_removed + 50.0f) < 0.01f &&
        fabsf(breakdown.progress + 0.05f) < 0.0001f) {
        fc_destroy(&state);
        return pass("Jad healer HP is ignored while Jad healing raises work");
    }

    char msg[160];
    snprintf(msg, sizeof(msg),
             "Jad healing progress wrong net=%.4f progress=%.6f",
             runtime.last_net_required_work_removed,
             breakdown.progress);
    fc_destroy(&state);
    return fail(msg);
}

static int test_net_progress_timer_and_clip_sanity(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_KET_ZEK, 12, 10, 0);
    state.npcs_remaining = 1;

    params = fc_reward_default_params();
    params.shape_no_progress_start_1 = 1;
    params.shape_no_progress_start_2 = 0;
    params.shape_no_progress_start_3 = 0;
    params.shape_no_progress_penalty_1 = -0.001f;
    fc_reward_runtime_begin_episode(&runtime, &state);

    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (runtime.ticks_since_positive_progress != 1 ||
        runtime.zero_progress_ticks != 1 ||
        breakdown.no_progress != 0.0f) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "first idle tick wrong timer=%d zero=%d no_progress=%.6f",
                 runtime.ticks_since_positive_progress,
                 runtime.zero_progress_ticks,
                 breakdown.no_progress);
        fc_destroy(&state);
        return fail(msg);
    }

    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (runtime.ticks_since_positive_progress != 2 ||
        breakdown.no_progress >= 0.0f) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "delayed no-progress penalty wrong timer=%d no_progress=%.6f",
                 runtime.ticks_since_positive_progress,
                 breakdown.no_progress);
        fc_destroy(&state);
        return fail(msg);
    }

    state.npcs[0].current_hp -= 100;
    breakdown = fc_reward_compute_breakdown(&state, &params, &runtime);
    if (runtime.ticks_since_positive_progress != 0 ||
        fabsf(breakdown.total) >= 1.0f) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "positive progress/reset or clip sanity wrong timer=%d total=%.6f",
                 runtime.ticks_since_positive_progress,
                 breakdown.total);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("net-progress timer delays penalties and ordinary progress does not clip");
}

static int test_progress_observation_fields(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    float obs[FC_OBS_SIZE];

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state.npcs_remaining = 1;

    memset(&params, 0, sizeof(params));
    params.w_progress = 0.001f;
    fc_reward_runtime_begin_episode(&runtime, &state);
    state.npcs[0].current_hp = 50;
    (void)fc_reward_compute_breakdown(&state, &params, &runtime);
    fc_reward_sync_progress_state(&state, &runtime);
    fc_write_obs(&state, obs);

    float cave = obs[FC_OBS_META_START + FC_OBS_META_CAVE_PROG];
    float wave = obs[FC_OBS_META_START + FC_OBS_META_WAVE_PROG];
    float work = obs[FC_OBS_META_START + FC_OBS_META_WORK_REM];
    float timer = obs[FC_OBS_META_START + FC_OBS_META_NO_PROG];

    if (fabsf(wave - 0.5f) < 0.0001f &&
        fabsf(work - 0.5f) < 0.0001f &&
        cave > 0.0f && cave < 0.01f &&
        fabsf(timer) < 0.0001f) {
        fc_destroy(&state);
        return pass("progress observation fields expose normalized progress state");
    }

    char msg[192];
    snprintf(msg, sizeof(msg),
             "progress obs wrong: cave=%.6f wave=%.6f work=%.6f timer=%.6f",
             cave, wave, work, timer);
    fc_destroy(&state);
    return fail(msg);
}

static int test_prayer_deadline_observation_fields(void) {
    FcState state;
    float obs[FC_OBS_SIZE];

    make_open_manual_state(&state, 10, 10);
    state.current_wave = FC_NUM_WAVES;
    state.tick = 100;
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 12, 10, 0);
    state.npcs_remaining = 1;

    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         500,
                         2,
                         ATTACK_RANGED,
                         0,
                         0);
    state.player.pending_hits[0].prayer_snapshot = -1;
    state.player.pending_hits[0].prayer_lock_tick = state.tick;

    fc_write_obs(&state, obs);

    int base = FC_OBS_NPC_START;
    float ranged_deadline = obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_RNG];
    float melee_deadline = obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_MEL];
    float magic_deadline = obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_MAG];
    float npc_window = obs[base + FC_NPC_PENDING_PRAYER_WINDOW];
    float npc_deadline = obs[base + FC_NPC_PENDING_PRAYER_DEADLINE];
    float pending_style = obs[base + FC_NPC_PENDING_STYLE];
    float pending_ticks = obs[base + FC_NPC_PENDING_TICKS];

    if (!(fabsf(ranged_deadline - 1.0f) < 0.0001f &&
          fabsf(melee_deadline) < 0.0001f &&
          fabsf(magic_deadline) < 0.0001f &&
          fabsf(npc_window - 1.0f) < 0.0001f &&
          fabsf(npc_deadline - 1.0f) < 0.0001f &&
          fabsf(pending_style - ((float)ATTACK_RANGED / 3.0f)) < 0.0001f &&
          fabsf(pending_ticks - 0.2f) < 0.0001f)) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "actionable deadline obs wrong: rng=%.3f mel=%.3f mag=%.3f window=%.3f deadline=%.3f style=%.3f ticks=%.3f",
                 ranged_deadline, melee_deadline, magic_deadline,
                 npc_window, npc_deadline, pending_style, pending_ticks);
        fc_destroy(&state);
        return fail(msg);
    }

    state.player.num_pending_hits = 0;
    memset(state.player.pending_hits, 0, sizeof(state.player.pending_hits));
    memset(obs, 0, sizeof(obs));
    state.tick = 200;
    state.npcs[0].npc_type = NPC_TOK_XIL;
    state.npcs[0].attack_style = ATTACK_RANGED;

    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         120,
                         2,
                         ATTACK_RANGED,
                         0,
                         0);
    state.player.pending_hits[0].prayer_snapshot = PRAYER_NONE;
    state.player.pending_hits[0].prayer_lock_tick = -1;

    fc_write_obs(&state, obs);

    ranged_deadline = obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_RNG];
    npc_window = obs[base + FC_NPC_PENDING_PRAYER_WINDOW];
    npc_deadline = obs[base + FC_NPC_PENDING_PRAYER_DEADLINE];
    pending_style = obs[base + FC_NPC_PENDING_STYLE];
    pending_ticks = obs[base + FC_NPC_PENDING_TICKS];

    if (fabsf(ranged_deadline) < 0.0001f &&
        fabsf(npc_window) < 0.0001f &&
        fabsf(npc_deadline) < 0.0001f &&
        fabsf(pending_style - ((float)ATTACK_RANGED / 3.0f)) < 0.0001f &&
        fabsf(pending_ticks - 0.2f) < 0.0001f) {
        fc_destroy(&state);
        return pass("prayer deadline obs distinguishes actionable and locked pending hits");
    }

    char msg[256];
    snprintf(msg, sizeof(msg),
             "locked pending hit deadline obs wrong: rng=%.3f window=%.3f deadline=%.3f style=%.3f ticks=%.3f",
             ranged_deadline, npc_window, npc_deadline, pending_style, pending_ticks);
    fc_destroy(&state);
    return fail(msg);
}

static int test_healer_spawn_validity(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    int healer_count = 0;
    int regions_seen[5] = {0};
    const int spawn_dirs[5] = {
        SPAWN_NORTH_WEST,
        SPAWN_SOUTH_WEST,
        SPAWN_SOUTH,
        SPAWN_SOUTH_EAST,
        SPAWN_CENTER,
    };

    make_open_manual_state(&state, 20, 20);
    state.current_wave = FC_NUM_WAVES;
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 10, 10, 0);
    state.npcs[0].current_hp = FC_JAD_HEALER_THRESHOLD_HP_TENTHS - 10;
    state.npcs_remaining = 1;
    state.next_spawn_index = 1;
    state.walkable[13][49] = 0;  /* force one regional spawn to relocate */

    fc_step(&state, actions);

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* n = &state.npcs[i];
        if (!n->active || n->is_dead || n->npc_type != NPC_YT_HURKOT) continue;
        healer_count++;
        if (!fc_footprint_walkable(n->x, n->y, n->size, state.walkable)) {
            char msg[160];
            snprintf(msg, sizeof(msg), "healer spawned on blocked footprint at (%d,%d)",
                     n->x, n->y);
            fc_destroy(&state);
            return fail(msg);
        }

        int best_region = -1;
        int best_distance = FC_ARENA_WIDTH;
        for (int region = 0; region < 5; region++) {
            int sx, sy;
            fc_spawn_position(spawn_dirs[region], &sx, &sy);
            int dx = abs(n->x - sx);
            int dy = abs(n->y - sy);
            int distance = dx > dy ? dx : dy;
            if (distance < best_distance) {
                best_distance = distance;
                best_region = region;
            }
        }
        if (best_region < 0 || best_distance > 5) {
            char msg[192];
            snprintf(msg, sizeof(msg),
                     "healer spawned outside Fight Cave regions at (%d,%d), nearest=%d",
                     n->x, n->y, best_distance);
            fc_destroy(&state);
            return fail(msg);
        }
        regions_seen[best_region] = 1;
    }

    int distinct_regions = 0;
    for (int i = 0; i < 5; i++) distinct_regions += regions_seen[i];

    if (healer_count == FC_JAD_NUM_HEALERS && distinct_regions == FC_JAD_NUM_HEALERS) {
        fc_destroy(&state);
        return pass("Jad healers use four valid non-north-east Fight Cave regions");
    }

    char msg[160];
    snprintf(msg, sizeof(msg), "expected %d healers in distinct regions, found %d in %d",
             FC_JAD_NUM_HEALERS, healer_count, distinct_regions);
    fc_destroy(&state);
    return fail(msg);
}

static int test_safespot_los(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    state.walkable[11][10] = 0;  /* Italy-rock-style blocker */
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 12, 10, 0);
    state.npcs_remaining = 1;
    state.current_wave = FC_NUM_WAVES;
    state.player.attack_target_idx = 0;
    state.player.approach_target = 1;

    fc_step(&state, actions);

    if (state.npcs[0].num_pending_hits != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "player fired through blocked LOS, pending hits=%d",
                 state.npcs[0].num_pending_hits);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.num_pending_hits != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Jad hit through blocked LOS, pending hits=%d",
                 state.player.num_pending_hits);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("blocked LOS prevents attacks through the safespot");
}

static int test_diagonal_corner_clipping(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    int invalid[FC_INVALID_ACTION_CLASS_COUNT] = {0};

    if (FC_MOVE_DX[FC_MOVE_RUN_NE] != 2 || FC_MOVE_DY[FC_MOVE_RUN_NE] != 2 ||
        FC_MOVE_DX[FC_MOVE_RUN_SE] != 2 || FC_MOVE_DY[FC_MOVE_RUN_SE] != -2 ||
        FC_MOVE_DX[FC_MOVE_RUN_SW] != -2 || FC_MOVE_DY[FC_MOVE_RUN_SW] != -2 ||
        FC_MOVE_DX[FC_MOVE_RUN_NW] != -2 || FC_MOVE_DY[FC_MOVE_RUN_NW] != 2) {
        return fail("diagonal run actions are not true two-tile diagonals");
    }

    make_open_manual_state(&state, 10, 10);
    state.walkable[11][10] = 0;
    actions[0] = FC_MOVE_WALK_NE;
    fc_step(&state, actions);
    if (state.player.x == 11 && state.player.y == 11) {
        fc_destroy(&state);
        return fail("player diagonal movement cut through a blocked corner");
    }
    if (state.player.x != 10 || state.player.y != 11) {
        char msg[128];
        snprintf(msg, sizeof(msg), "expected player fallback to north, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    state.walkable[11][10] = 0;
    state.walkable[10][11] = 0;
    actions[0] = FC_MOVE_WALK_NE;
    fc_action_invalid_classes(&state, actions, invalid);
    if (!invalid[FC_INVALID_ACTION_MOVE]) {
        fc_destroy(&state);
        return fail("blocked diagonal move was not masked invalid");
    }
    fc_step(&state, actions);
    if (state.player.x != 10 || state.player.y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "blocked diagonal moved player to (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    int nx = 10;
    int ny = 10;
    state.walkable[11][10] = 0;
    if (!fc_npc_step_toward_sized(&nx, &ny, 12, 12, 2, state.walkable)) {
        fc_destroy(&state);
        return fail("sized NPC failed to take cardinal fallback around blocked corner");
    }
    if (nx == 11 && ny == 11) {
        fc_destroy(&state);
        return fail("sized NPC diagonal movement cut through a blocked side footprint");
    }
    fc_destroy(&state);

    return pass("diagonal movement obeys side-tile clipping");
}

static int test_npc_moves_when_attack_blocked(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    state.walkable[11][10] = 0;
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, 12, 11, 0);
    state.npcs[0].size = 1;
    state.npcs_remaining = 1;

    fc_step(&state, actions);
    if (state.npcs[0].x != 11 || state.npcs[0].y != 11) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "ranged NPC with blocked LOS did not move toward a valid firing tile; got (%d,%d)",
                 state.npcs[0].x, state.npcs[0].y);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    state.walkable[11][10] = 0;
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 11, 0);
    state.npcs_remaining = 1;

    fc_step(&state, actions);
    if (state.npcs[0].x != 10 || state.npcs[0].y != 11) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "melee NPC with blocked diagonal contact did not step to cardinal contact; got (%d,%d)",
                 state.npcs[0].x, state.npcs[0].y);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    return pass("NPCs keep moving when attack position is blocked");
}

static int test_option_b_no_move_into_range_fire_same_tick(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 3);
    spawn_static_guardrail_target(&state, 14, 10);

    int pre_dist = fc_distance_to_npc(state.player.x, state.player.y, &state.npcs[0]);
    int post_dist = fc_distance_to_npc(11, 10, &state.npcs[0]);
    if (pre_dist <= state.player.weapon_range || post_dist > state.player.weapon_range) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "setup expected out-of-range before move and in-range after move, got pre=%d post=%d range=%d",
                 pre_dist, post_dist, state.player.weapon_range);
        fc_destroy(&state);
        return fail(msg);
    }

    actions[0] = FC_MOVE_WALK_E;
    actions[1] = 1;
    fc_step(&state, actions);

    if (state.attack_attempt_this_tick) {
        fc_destroy(&state);
        return fail("player moved into range and fired in the same tick");
    }
    if (state.player.x != 11 || state.player.y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "expected player movement to still apply, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("moving into range does not allow same-tick firing");
}

static int test_option_b_no_move_into_los_fire_same_tick(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 5, 5);
    configure_option_b_player(&state, 5);
    spawn_static_guardrail_target(&state, 5, 7);
    state.walkable[5][6] = 0;

    int pre_los = fc_has_los_to_npc(5, 5, state.npcs[0].x, state.npcs[0].y,
                                    state.npcs[0].size, state.walkable);
    int post_los = fc_has_los_to_npc(6, 5, state.npcs[0].x, state.npcs[0].y,
                                     state.npcs[0].size, state.walkable);
    if (pre_los || !post_los) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "setup expected blocked LOS before move and clear LOS after move, got pre=%d post=%d",
                 pre_los, post_los);
        fc_destroy(&state);
        return fail(msg);
    }

    actions[0] = FC_MOVE_WALK_E;
    actions[1] = 1;
    fc_step(&state, actions);

    if (state.attack_attempt_this_tick) {
        fc_destroy(&state);
        return fail("player moved into LOS and fired in the same tick");
    }
    if (state.player.x != 6 || state.player.y != 5) {
        char msg[128];
        snprintf(msg, sizeof(msg), "expected player movement to still apply, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("moving into LOS does not allow same-tick firing");
}

static int test_option_b_attack_then_move_when_already_valid(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 2);
    spawn_static_guardrail_target(&state, 12, 10);

    int pre_dist = fc_distance_to_npc(state.player.x, state.player.y, &state.npcs[0]);
    int pre_los = fc_has_los_to_npc(state.player.x, state.player.y,
                                    state.npcs[0].x, state.npcs[0].y,
                                    state.npcs[0].size, state.walkable);
    int post_dist = fc_distance_to_npc(9, 10, &state.npcs[0]);
    if (pre_dist > state.player.weapon_range || !pre_los ||
        post_dist <= state.player.weapon_range) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "setup expected valid attack before move and out-of-range after move, got pre_dist=%d pre_los=%d post_dist=%d range=%d",
                 pre_dist, pre_los, post_dist, state.player.weapon_range);
        fc_destroy(&state);
        return fail(msg);
    }

    actions[0] = FC_MOVE_WALK_W;
    actions[1] = 1;
    fc_step(&state, actions);

    if (!state.attack_attempt_this_tick) {
        fc_destroy(&state);
        return fail("player failed to attack before later same-tick movement");
    }
    if (state.player.x != 9 || state.player.y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "expected player movement after attack, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("attack can fire before later same-tick movement when already valid");
}

static int test_option_b_queued_projectile_survives_later_movement(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 5);
    spawn_static_guardrail_target(&state, 12, 10);

    if (!fc_queue_pending_hit(state.npcs[0].pending_hits,
                              &state.npcs[0].num_pending_hits,
                              FC_MAX_PENDING_HITS,
                              50, 3, ATTACK_RANGED, -1, 0)) {
        fc_destroy(&state);
        return fail("setup failed to queue delayed player projectile");
    }

    actions[0] = FC_MOVE_WALK_W;
    fc_step(&state, actions);

    if (state.player.x != 9 || state.player.y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "expected player movement while projectile in flight, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.npcs[0].num_pending_hits != 1) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "queued projectile was lost after movement; pending hits=%d",
                 state.npcs[0].num_pending_hits);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.npcs[0].pending_hits[0].ticks_remaining != 2) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "queued projectile did not tick down predictably; ticks_remaining=%d",
                 state.npcs[0].pending_hits[0].ticks_remaining);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("queued projectile remains in flight after later movement");
}

static int test_step1_movement_only_clears_stale_target(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 5);
    spawn_static_guardrail_target(&state, 12, 10);
    state.player.attack_target_idx = 0;
    state.player.approach_target = 1;

    actions[0] = FC_MOVE_WALK_W;
    fc_step(&state, actions);

    if (state.attack_attempt_this_tick || state.npcs[0].num_pending_hits != 0) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "movement-only auto-fired stale target: attack=%d pending=%d",
                 state.attack_attempt_this_tick, state.npcs[0].num_pending_hits);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.x != 9 || state.player.y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "movement-only did not move west, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.attack_target_idx != -1 || state.player.approach_target != 0) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "stale combat state not cleared: target=%d approach=%d",
                 state.player.attack_target_idx, state.player.approach_target);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("movement-only clears stale target without auto-firing");
}

static int test_step1_projectile_survives_movement_cancel(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 5);
    spawn_static_guardrail_target(&state, 12, 10);
    state.player.attack_target_idx = 0;
    state.player.approach_target = 1;

    int before_hp = state.npcs[0].current_hp;
    if (!fc_queue_pending_hit(state.npcs[0].pending_hits,
                              &state.npcs[0].num_pending_hits,
                              FC_MAX_PENDING_HITS,
                              50, 1, ATTACK_RANGED, -1, 0)) {
        fc_destroy(&state);
        return fail("setup failed to queue committed projectile");
    }

    actions[0] = FC_MOVE_WALK_W;
    fc_step(&state, actions);

    if (state.attack_attempt_this_tick) {
        fc_destroy(&state);
        return fail("movement cancel created a new attack while projectile was pending");
    }
    if (state.npcs[0].current_hp != before_hp - 50 ||
        state.damage_dealt_this_tick != 50 ||
        state.npcs[0].num_pending_hits != 0) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "committed projectile did not resolve: hp_before=%d hp_after=%d damage=%d pending=%d",
                 before_hp, state.npcs[0].current_hp,
                 state.damage_dealt_this_tick, state.npcs[0].num_pending_hits);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.attack_target_idx != -1 || state.player.approach_target != 0) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "movement cancel did not clear combat intent: target=%d approach=%d",
                 state.player.attack_target_idx, state.player.approach_target);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("movement cancel clears intent but committed projectile still lands");
}

static int test_step1_attack_move_clears_continued_target(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 2);
    spawn_static_guardrail_target(&state, 12, 10);

    actions[0] = FC_MOVE_WALK_W;
    actions[1] = 1;
    fc_step(&state, actions);

    if (!state.attack_attempt_this_tick) {
        fc_destroy(&state);
        return fail("attack+move failed to fire from valid pre-movement tile");
    }
    if (state.player.x != 9 || state.player.y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "attack+move did not apply movement, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.attack_target_idx != -1 || state.player.approach_target != 0 ||
        state.player.route_idx != 0 || state.player.route_len != 0) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "attack+move kept continued intent: target=%d approach=%d route=%d/%d",
                 state.player.attack_target_idx, state.player.approach_target,
                 state.player.route_idx, state.player.route_len);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("attack+move fires once then clears continued target intent");
}

static int test_step1_attack_move_out_of_range_clears_target(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 3);
    spawn_static_guardrail_target(&state, 14, 10);

    actions[0] = FC_MOVE_WALK_E;
    actions[1] = 1;
    fc_step(&state, actions);

    if (state.attack_attempt_this_tick) {
        fc_destroy(&state);
        return fail("attack+move fired after moving into range");
    }
    if (state.player.x != 11 || state.player.y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "attack+move did not apply movement, got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.attack_target_idx != -1 || state.player.approach_target != 0 ||
        state.player.route_idx != 0 || state.player.route_len != 0) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "failed attack+move kept continued intent: target=%d approach=%d route=%d/%d",
                 state.player.attack_target_idx, state.player.approach_target,
                 state.player.route_idx, state.player.route_len);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("attack+move cannot move into range and clears continued target intent");
}

static int test_step1_directional_cancels_old_approach_route(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 3);
    spawn_static_guardrail_target(&state, 14, 10);
    state.player.is_running = 0;
    state.player.attack_target_idx = 0;
    state.player.approach_target = 1;
    state.player.route_x[0] = 11;
    state.player.route_y[0] = 10;
    state.player.route_len = 1;
    state.player.route_idx = 0;

    actions[0] = FC_MOVE_WALK_W;
    fc_step(&state, actions);

    if (state.player.x != 9 || state.player.y != 10) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "directional move followed old approach route instead; got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.route_len != 0 || state.player.route_idx != 0 ||
        state.player.approach_target != 0 || state.player.attack_target_idx != -1) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "old approach state not cleared: route=%d/%d approach=%d target=%d",
                 state.player.route_idx, state.player.route_len,
                 state.player.approach_target, state.player.attack_target_idx);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("directional movement cancels old combat approach route");
}

static int test_step1_directional_beats_old_tile_route(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    state.player.is_running = 0;
    state.player.route_x[0] = 11;
    state.player.route_y[0] = 10;
    state.player.route_len = 1;
    state.player.route_idx = 0;

    actions[0] = FC_MOVE_WALK_N;
    fc_step(&state, actions);

    if (state.player.x != 10 || state.player.y != 11) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "directional move followed old tile route instead; got (%d,%d)",
                 state.player.x, state.player.y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.route_len != 0 || state.player.route_idx != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "old tile route not cleared: route=%d/%d",
                 state.player.route_idx, state.player.route_len);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("directional movement overrides old walk-to-tile route");
}

static int test_step1_attack_approach_without_explicit_movement(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    configure_option_b_player(&state, 3);
    state.player.is_running = 0;
    spawn_static_guardrail_target(&state, 16, 10);
    int pre_dist = fc_distance_to_npc(state.player.x, state.player.y, &state.npcs[0]);

    actions[1] = 1;
    fc_step(&state, actions);

    if (state.attack_attempt_this_tick) {
        fc_destroy(&state);
        return fail("out-of-range attack approach fired immediately");
    }
    int post_dist = fc_distance_to_npc(state.player.x, state.player.y, &state.npcs[0]);
    if ((state.player.x == 10 && state.player.y == 10) || post_dist >= pre_dist) {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "attack approach did not move closer, got (%d,%d) dist %d -> %d",
                 state.player.x, state.player.y, pre_dist, post_dist);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.attack_target_idx != 0 || state.player.approach_target != 1 ||
        state.player.route_len <= state.player.route_idx) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "attack approach state wrong: target=%d approach=%d route=%d/%d",
                 state.player.attack_target_idx, state.player.approach_target,
                 state.player.route_idx, state.player.route_len);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("attack approach still works without explicit movement");
}

static int test_step2_occupancy_marks_and_ignores_entities(void) {
    FcState state;
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, 12, 12, 0);
    state.npcs[0].size = 2;
    state.npcs_remaining = 1;

    fc_build_occupancy(&state, occupied, -1, 0);
    if (!occupied[10][10] ||
        !occupied[12][12] || !occupied[13][12] ||
        !occupied[12][13] || !occupied[13][13] ||
        occupied[14][14]) {
        fc_destroy(&state);
        return fail("occupancy did not mark player and full NPC footprint correctly");
    }

    fc_build_occupancy(&state, occupied, 0, 0);
    if (!occupied[10][10] || occupied[12][12] || occupied[13][13]) {
        fc_destroy(&state);
        return fail("occupancy did not ignore moving NPC footprint");
    }

    fc_build_occupancy(&state, occupied, -1, 1);
    if (occupied[10][10] || !occupied[12][12] || !occupied[13][13]) {
        fc_destroy(&state);
        return fail("occupancy did not ignore player while preserving NPC footprint");
    }

    fc_destroy(&state);
    return pass("occupancy marks player/NPC footprints and supports ignore flags");
}

static int test_step2_dynamic_footprint_static_and_occupied(void) {
    FcState state;
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];

    make_open_manual_state(&state, 10, 10);
    fc_clear_occupancy(occupied);
    state.walkable[5][5] = 0;
    fc_mark_footprint_occupied(occupied, 6, 6, 1);

    if (fc_footprint_available_dynamic(4, 4, 2, state.walkable, occupied)) {
        fc_destroy(&state);
        return fail("dynamic footprint allowed static blocked terrain");
    }
    if (fc_footprint_available_dynamic(6, 6, 1, state.walkable, occupied)) {
        fc_destroy(&state);
        return fail("dynamic footprint allowed occupied tile");
    }
    if (!fc_footprint_available_dynamic(7, 7, 2, state.walkable, occupied)) {
        fc_destroy(&state);
        return fail("dynamic footprint rejected clear static+dynamic area");
    }

    fc_destroy(&state);
    return pass("dynamic footprint checks static terrain and occupancy");
}

static int test_step2_entity_wrapper_ignores_self_blocks_others(void) {
    FcState state;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, 12, 12, 0);
    state.npcs[0].size = 2;
    fc_npc_spawn(&state.npcs[1], NPC_TZ_KIH, 15, 12, 1);
    state.npcs_remaining = 2;

    if (!fc_footprint_available_for_entity(&state, 12, 12, 2, 0, 0)) {
        fc_destroy(&state);
        return fail("entity availability did not ignore moving NPC's own footprint");
    }
    if (fc_footprint_available_for_entity(&state, 15, 12, 2, 0, 0)) {
        fc_destroy(&state);
        return fail("entity availability allowed overlap with another NPC");
    }
    if (fc_footprint_available_for_entity(&state, 10, 10, 1, 0, 0)) {
        fc_destroy(&state);
        return fail("entity availability allowed overlap with player tile");
    }
    if (!fc_footprint_available_for_entity(&state, 10, 10, 1, 0, 1)) {
        fc_destroy(&state);
        return fail("entity availability did not honor ignore_player");
    }

    fc_destroy(&state);
    return pass("entity availability ignores self while blocking player/other NPCs");
}

static int test_step2_dynamic_diagonal_blocks_occupied_corner(void) {
    FcState state;
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];
    int x = 10;
    int y = 10;

    make_open_manual_state(&state, 10, 10);
    fc_clear_occupancy(occupied);
    fc_mark_footprint_occupied(occupied, 11, 10, 1);
    fc_mark_footprint_occupied(occupied, 10, 11, 1);

    int moved = fc_npc_step_toward_sized_dynamic(&x, &y, 11, 11, 1,
                                                 state.walkable, occupied);
    if (moved || x != 10 || y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "dynamic diagonal clipped through occupied corner to (%d,%d)",
                 x, y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("dynamic diagonal movement rejects occupied corner clipping");
}

static int test_step2_dynamic_bfs_avoids_occupied_steps(void) {
    FcState state;
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];
    int out_x[FC_MAX_ROUTE];
    int out_y[FC_MAX_ROUTE];

    make_open_manual_state(&state, 10, 10);
    fc_clear_occupancy(occupied);
    fc_mark_footprint_occupied(occupied, 11, 10, 1);

    int steps = fc_pathfind_bfs_sized_dynamic(10, 10, 12, 10, 1,
                                              state.walkable, occupied,
                                              out_x, out_y, FC_MAX_ROUTE);
    if (steps <= 0 || out_x[steps - 1] != 12 || out_y[steps - 1] != 10) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "dynamic BFS failed to route around occupied tile, steps=%d last=(%d,%d)",
                 steps, steps > 0 ? out_x[steps - 1] : -1,
                 steps > 0 ? out_y[steps - 1] : -1);
        fc_destroy(&state);
        return fail(msg);
    }
    for (int i = 0; i < steps; i++) {
        if (out_x[i] == 11 && out_y[i] == 10) {
            fc_destroy(&state);
            return fail("dynamic BFS route included occupied intermediate tile");
        }
    }

    fc_destroy(&state);
    return pass("dynamic sized BFS avoids occupied route steps");
}

static int test_step2_dynamic_sized_bfs_checks_full_footprint(void) {
    FcState state;
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];
    int out_x[FC_MAX_ROUTE];
    int out_y[FC_MAX_ROUTE];

    make_open_manual_state(&state, 10, 10);
    fc_clear_occupancy(occupied);
    fc_mark_footprint_occupied(occupied, 13, 10, 1);

    int steps = fc_pathfind_bfs_sized_dynamic(10, 10, 12, 10, 2,
                                              state.walkable, occupied,
                                              out_x, out_y, FC_MAX_ROUTE);
    if (steps != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "sized dynamic BFS accepted occupied destination footprint, steps=%d",
                 steps);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_clear_occupancy(occupied);
    steps = fc_pathfind_bfs_sized_dynamic(10, 10, 12, 10, 2,
                                          state.walkable, occupied,
                                          out_x, out_y, FC_MAX_ROUTE);
    if (steps <= 0 || out_x[steps - 1] != 12 || out_y[steps - 1] != 10) {
        fc_destroy(&state);
        return fail("sized dynamic BFS rejected clear full-footprint route");
    }

    fc_destroy(&state);
    return pass("dynamic sized BFS validates full destination footprint");
}

static int test_step2_start_reservation_blocks_swap_tile(void) {
    FcState state;
    uint8_t occupied[FC_ARENA_WIDTH][FC_ARENA_HEIGHT];
    int x = 11;
    int y = 10;

    make_open_manual_state(&state, 10, 10);
    fc_clear_occupancy(occupied);
    fc_mark_footprint_occupied(occupied, 10, 10, 1);

    int moved = fc_npc_step_toward_sized_dynamic(&x, &y, 10, 10, 1,
                                                 state.walkable, occupied);
    if (moved || x != 11 || y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "dynamic movement entered reserved start tile, moved=%d pos=(%d,%d)",
                 moved, x, y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("start-of-tick reservation occupancy blocks swap-through tile");
}

static int test_step3_player_directional_blocked_by_npc(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.npcs[0].movement_speed = 0;
    state.npcs[0].attack_timer = 999;
    state.npcs_remaining = 1;

    actions[0] = FC_MOVE_WALK_E;
    fc_step(&state, actions);

    if (state.player.x != 10 || state.player.y != 10) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "player moved into NPC footprint, got player=(%d,%d) npc=(%d,%d)",
                 state.player.x, state.player.y, state.npcs[0].x, state.npcs[0].y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("player directional movement is blocked by NPC footprint");
}

static int test_step3_player_tile_route_rejects_occupied_target(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.npcs[0].movement_speed = 0;
    state.npcs[0].attack_timer = 999;
    state.npcs_remaining = 1;

    actions[5] = 12;  /* target x = 11 */
    actions[6] = 11;  /* target y = 10 */
    fc_step(&state, actions);

    if (state.player.x == 11 && state.player.y == 10) {
        fc_destroy(&state);
        return fail("walk-to-tile route moved player onto occupied NPC tile");
    }
    if (state.player.route_len != 0 || state.player.route_idx != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "occupied tile route was retained: route=%d/%d",
                 state.player.route_idx, state.player.route_len);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("walk-to-tile routing rejects occupied target tile");
}

static int test_step3_npc_blocked_by_other_npc(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    state.walkable[12][11] = 0;
    state.walkable[12][9] = 0;
    state.walkable[13][10] = 0;
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_TZ_KIH, 12, 10, 1);
    state.npcs[0].attack_timer = 999;
    state.npcs[1].attack_timer = 999;
    state.npcs_remaining = 2;

    fc_step(&state, actions);

    if (state.npcs[1].x == state.npcs[0].x &&
        state.npcs[1].y == state.npcs[0].y) {
        fc_destroy(&state);
        return fail("back NPC stepped into front NPC footprint");
    }
    if (state.npcs[1].x != 12 || state.npcs[1].y != 10) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "back NPC should be body-blocked at (12,10), got (%d,%d)",
                 state.npcs[1].x, state.npcs[1].y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("NPC movement is blocked by another NPC footprint");
}

static int test_step3_large_npc_blocked_by_healer_footprint(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 30, 10);
    state.current_wave = FC_NUM_WAVES;
    state.walkable[10][15] = 0;
    state.walkable[10][9] = 0;
    state.walkable[9][10] = 0;
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 10, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_YT_HURKOT, 15, 10, 1);
    state.npcs[0].attack_timer = 999;
    state.npcs[1].attack_timer = 999;
    state.npcs_remaining = 2;

    fc_step(&state, actions);

    if (guardrail_footprints_overlap(state.npcs[0].x, state.npcs[0].y,
                                     state.npcs[0].size,
                                     state.npcs[1].x, state.npcs[1].y,
                                     state.npcs[1].size)) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "large NPC overlapped healer footprint: jad=(%d,%d) healer=(%d,%d)",
                 state.npcs[0].x, state.npcs[0].y,
                 state.npcs[1].x, state.npcs[1].y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.npcs[0].x != 10 || state.npcs[0].y != 10) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Jad should be blocked by healer, got (%d,%d)",
                 state.npcs[0].x, state.npcs[0].y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("large NPC movement is blocked by healer footprint");
}

static int test_step3_tz_kek_split_avoids_occupied_tiles(void) {
    FcState state;
    int child_count = 0;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.npcs_remaining = 1;
    state.next_spawn_index = 1;

    fc_npc_tz_kek_split(&state, 10, 10);

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        FcNpc* n = &state.npcs[i];
        if (!n->active || n->is_dead || n->npc_type != NPC_TZ_KEK_SM) continue;
        child_count++;
        if (guardrail_footprints_overlap(n->x, n->y, n->size,
                                         state.player.x, state.player.y, 1)) {
            fc_destroy(&state);
            return fail("Tz-Kek split child spawned on player tile");
        }
        if (guardrail_footprints_overlap(n->x, n->y, n->size,
                                         state.npcs[0].x, state.npcs[0].y,
                                         state.npcs[0].size)) {
            fc_destroy(&state);
            return fail("Tz-Kek split child spawned on occupied NPC tile");
        }
        if (!fc_footprint_walkable(n->x, n->y, n->size, state.walkable)) {
            fc_destroy(&state);
            return fail("Tz-Kek split child spawned on blocked terrain");
        }
    }

    if (child_count != 2) {
        char msg[128];
        snprintf(msg, sizeof(msg), "expected 2 split children, got %d", child_count);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("Tz-Kek split children avoid player/NPC/wall occupancy");
}

static int test_step4_ranged_npc_chases_player_bounds_not_los_tile(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    state.walkable[12][10] = 0;  /* blocks direct LOS on the horizontal lane */
    state.walkable[13][9] = 0;   /* old attack-position search would sidestep south */
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, 14, 10, 0);
    state.npcs[0].size = 1;
    state.npcs[0].attack_timer = 999;
    state.npcs_remaining = 1;

    fc_step(&state, actions);

    if (state.npcs[0].x != 13 || state.npcs[0].y != 10) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "ranged NPC did not chase player bounds; got (%d,%d)",
                 state.npcs[0].x, state.npcs[0].y);
        fc_destroy(&state);
        return fail(msg);
    }
    if (state.player.num_pending_hits != 0) {
        fc_destroy(&state);
        return fail("ranged NPC attacked after chasing to a blocked-LOS tile");
    }

    fc_destroy(&state);
    return pass("ranged NPC chases player bounds instead of solving for LOS tile");
}

static int test_step4_large_npc_chase_checks_full_footprint(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    state.walkable[24][10] = 0;  /* direct west step footprint is blocked */
    fc_npc_spawn(&state.npcs[0], NPC_KET_ZEK, 25, 10, 0);
    state.npcs[0].attack_timer = 999;
    state.npcs_remaining = 1;

    fc_step(&state, actions);

    if (state.npcs[0].x != 25 || state.npcs[0].y != 10) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "large NPC moved despite blocked direct chase footprint; got (%d,%d)",
                 state.npcs[0].x, state.npcs[0].y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("large NPC chase movement checks the full direct-step footprint");
}

static int test_step4_npc_stays_when_current_position_can_attack(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS] = {0};

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TOK_XIL, 20, 10, 0);
    state.npcs[0].size = 1;
    state.npcs[0].attack_timer = 999;
    state.npcs_remaining = 1;

    if (!fc_npc_position_can_attack_player(&state, &state.npcs[0],
                                           state.npcs[0].x, state.npcs[0].y)) {
        fc_destroy(&state);
        return fail("setup expected current NPC position to be attack-capable");
    }

    fc_step(&state, actions);

    if (state.npcs[0].x != 20 || state.npcs[0].y != 10) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "NPC moved even though current tile could attack; got (%d,%d)",
                 state.npcs[0].x, state.npcs[0].y);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("NPCs stay put when their current footprint can attack");
}

static int test_special_tz_kih_prayer_drain(void) {
    FcState state;
    const FcNpcStats* stats = fc_npc_get_stats(NPC_TZ_KIH);
    int prayer_before;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.player.prayer = PRAYER_NONE;
    prayer_before = state.player.current_prayer;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         30, 1, ATTACK_MELEE, 0, stats->prayer_drain);
    fc_resolve_player_pending_hits(&state);

    if (prayer_before - state.player.current_prayer != 40 ||
        state.prayer_lost_this_tick != 40 ||
        state.tz_kih_prayer_drain_this_tick != 40) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "3.0 damage should drain 4.0 prayer; actual=%d aggregate=%d Tz-Kih=%d",
                 prayer_before - state.player.current_prayer,
                 state.prayer_lost_this_tick,
                 state.tz_kih_prayer_drain_this_tick);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.player.prayer = PRAYER_PROTECT_MELEE;
    prayer_before = state.player.current_prayer;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         30, 1, ATTACK_MELEE, 0, stats->prayer_drain);
    state.player.pending_hits[0].prayer_snapshot = PRAYER_PROTECT_MELEE;
    fc_resolve_player_pending_hits(&state);

    if (prayer_before - state.player.current_prayer != 10 ||
        state.player.damage_taken_this_tick != 0 ||
        state.tz_kih_prayer_drain_this_tick != 10) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "blocked Tz-Kih hit should deal 0 and drain 1.0 prayer; damage=%d drain=%d",
                 state.player.damage_taken_this_tick,
                 prayer_before - state.player.current_prayer);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("Tz-Kih drains damage dealt plus one prayer point");
}

static int test_special_mejkot_heal_replaces_attack(void) {
    FcState state;
    int hp_before;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_MEJKOT, 11, 10, 0);
    state.npcs[0].current_hp = state.npcs[0].max_hp / 4;
    state.npcs[0].attack_timer = 0;
    state.npcs[0].heal_timer = 0;
    hp_before = state.npcs[0].current_hp;
    fc_npc_tick(&state, 0);

    if (state.npcs[0].current_hp != hp_before + state.npcs[0].heal_amount ||
        state.npcs[0].healing_received_this_tick != state.npcs[0].heal_amount ||
        state.player.num_pending_hits != 0 ||
        state.npcs[0].attack_timer != state.npcs[0].attack_speed) {
        char msg[224];
        snprintf(msg, sizeof(msg),
                 "self heal did not replace attack: hp=%d expected=%d hits=%d timer=%d",
                 state.npcs[0].current_hp, hp_before + state.npcs[0].heal_amount,
                 state.player.num_pending_hits, state.npcs[0].attack_timer);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_MEJKOT, 11, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_TOK_XIL, 16, 10, 1);
    state.npcs[0].attack_timer = 0;
    state.npcs[0].heal_timer = 0;
    state.npcs[1].current_hp = state.npcs[1].max_hp / 4;
    hp_before = state.npcs[1].current_hp;
    fc_npc_tick(&state, 0);

    if (state.npcs[1].current_hp != hp_before + state.npcs[0].heal_amount ||
        state.npcs[1].healing_received_this_tick != state.npcs[0].heal_amount ||
        state.player.num_pending_hits != 0) {
        char msg[224];
        snprintf(msg, sizeof(msg),
                 "other-NPC heal did not replace attack: hp=%d expected=%d hits=%d",
                 state.npcs[1].current_hp, hp_before + state.npcs[0].heal_amount,
                 state.player.num_pending_hits);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_MEJKOT, 11, 10, 0);
    state.npcs[0].attack_timer = 0;
    state.npcs[0].heal_timer = 0;
    fc_npc_tick(&state, 0);
    if (state.player.num_pending_hits != 1) {
        char msg[128];
        snprintf(msg, sizeof(msg), "healthy MejKot queued %d melee hits, expected 1",
                 state.player.num_pending_hits);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("Yt-MejKot self/other healing consumes its attack cycle");
}

static int count_adjacent_styles(int npc_type, int counts[4]) {
    FcState state;

    memset(counts, 0, sizeof(int) * 4);
    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], npc_type, 11, 10, 0);

    for (int i = 0; i < 256; i++) {
        state.npcs[0].attack_timer = 0;
        state.player.num_pending_hits = 0;
        memset(state.player.pending_hits, 0, sizeof(state.player.pending_hits));
        fc_npc_tick(&state, 0);
        if (state.player.num_pending_hits != 1) {
            fc_destroy(&state);
            return 1;
        }
        int style = state.player.pending_hits[0].attack_style;
        if (style >= ATTACK_NONE && style <= ATTACK_MAGIC) counts[style]++;
    }

    fc_destroy(&state);
    return 0;
}

static int test_special_adjacent_style_selection(void) {
    int ket[4];
    int jad[4];

    if (count_adjacent_styles(NPC_KET_ZEK, ket)) {
        return fail("Ket-Zek did not queue exactly one adjacent attack per cycle");
    }
    if (ket[ATTACK_MELEE] == 0 || ket[ATTACK_MAGIC] == 0 ||
        ket[ATTACK_RANGED] != 0) {
        char msg[160];
        snprintf(msg, sizeof(msg), "adjacent Ket-Zek M/R/A=%d/%d/%d",
                 ket[ATTACK_MELEE], ket[ATTACK_RANGED], ket[ATTACK_MAGIC]);
        return fail(msg);
    }

    if (count_adjacent_styles(NPC_TZTOK_JAD, jad)) {
        return fail("Jad did not queue exactly one adjacent attack per cycle");
    }
    if (jad[ATTACK_MELEE] == 0 || jad[ATTACK_RANGED] == 0 ||
        jad[ATTACK_MAGIC] == 0) {
        char msg[160];
        snprintf(msg, sizeof(msg), "adjacent Jad M/R/A=%d/%d/%d",
                 jad[ATTACK_MELEE], jad[ATTACK_RANGED], jad[ATTACK_MAGIC]);
        return fail(msg);
    }
    if (fc_npc_hit_delay(NPC_TZTOK_JAD, ATTACK_MAGIC, 1) < 2) {
        return fail("adjacent Jad Magic resolves before a prayer decision tick");
    }

    return pass("Ket-Zek and Jad retain all valid styles in melee range");
}

static int test_special_hurkot_behavior(void) {
    FcState state;
    float obs[FC_OBS_SIZE];
    int hp_before;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 12, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_YT_HURKOT, 11, 10, 1);
    state.npcs[0].current_hp = 1000;
    state.npcs[1].attack_timer = 0;
    state.npcs[1].heal_timer = 1;
    fc_queue_pending_hit(state.npcs[1].pending_hits,
                         &state.npcs[1].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         0, 1, ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, 1);
    hp_before = state.npcs[0].current_hp;
    fc_npc_tick(&state, 1);

    if (!state.npcs[1].healer_distracted ||
        state.player.num_pending_hits != 1 ||
        state.npcs[0].current_hp != hp_before ||
        state.npc_heal_procs_this_tick != 0) {
        char msg[224];
        snprintf(msg, sizeof(msg),
                 "tagged healer aggro/no-heal wrong: aggro=%d hits=%d jad_hp=%d expected=%d",
                 state.npcs[1].healer_distracted, state.player.num_pending_hits,
                 state.npcs[0].current_hp, hp_before);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 30, 30);
    fc_npc_spawn(&state.npcs[0], NPC_YT_HURKOT, 10, 10, 0);
    state.npcs[0].healer_distracted = 1;
    fc_write_obs(&state, obs);
    if (obs[FC_OBS_NPC_START + FC_NPC_TELE_MELEE] < 0.5f) {
        fc_destroy(&state);
        return fail("tagged Yt-HurKot does not expose its melee threat");
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 40, 40);
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 10, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_YT_HURKOT, 15, 10, 1);
    state.npcs[1].healer_distracted = 1;
    state.npcs[0].current_hp = 1000;
    state.npcs[1].heal_timer = 1;
    state.npcs[1].attack_timer = 999;
    hp_before = state.npcs[0].current_hp;
    fc_npc_tick(&state, 1);
    if (!state.npcs[1].healer_distracted ||
        state.npcs[0].current_hp != hp_before) {
        fc_destroy(&state);
        return fail("distant tagged healer lost permanent aggro or healed Jad");
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_HURKOT, 12, 10, 0);
    state.npcs[0].healer_distracted = 1;
    state.walkable[11][10] = 0;
    fc_npc_tick(&state, 0);
    if (state.npcs[0].x != 12 || state.npcs[0].y != 10 ||
        !state.npcs[0].healer_distracted) {
        fc_destroy(&state);
        return fail("tagged healer routed around a blocking pillar or lost aggro");
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 30, 30);
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 10, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_YT_HURKOT, 15, 10, 1);
    state.npcs[0].current_hp = 1000;
    state.npcs[1].heal_timer = 4;
    hp_before = state.npcs[0].current_hp;
    for (int tick = 0; tick < 4; tick++) fc_npc_tick(&state, 1);
    if (state.npcs[0].current_hp != hp_before + state.npcs[1].heal_amount) {
        char msg[160];
        snprintf(msg, sizeof(msg), "four-tick healer restored %d, expected %d",
                 state.npcs[0].current_hp - hp_before, state.npcs[1].heal_amount);
        fc_destroy(&state);
        return fail(msg);
    }

    fc_destroy(&state);
    return pass("Yt-HurKot aggro is permanent, pillar-trappable, and disables healing");
}

static int test_mechanics_observation_events(void) {
    FcState state;
    float obs[FC_OBS_SIZE];
    int actions[FC_NUM_ACTION_HEADS] = {0};
    const float eps = 0.0001f;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    state.npcs_remaining = 1;
    state.player.prayer = PRAYER_NONE;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         30, 1, ATTACK_MELEE, 0,
                         fc_npc_get_stats(NPC_TZ_KIH)->prayer_drain);
    fc_resolve_player_pending_hits(&state);
    fc_write_obs(&state, obs);

    float expected_total = 40.0f / (float)state.player.max_prayer;
    float expected_source = 40.0f / 50.0f;
    if (fabsf(obs[FC_OBS_PLAYER_PRAYER_LOST] - expected_total) > eps ||
        obs[FC_OBS_PLAYER_OVERHEAD_PRAYER_LOST] != 0.0f ||
        fabsf(obs[FC_OBS_NPC_START + FC_NPC_PRAYER_DRAIN_DEALT] -
              expected_source) > eps) {
        char msg[224];
        snprintf(msg, sizeof(msg),
                 "Tz-Kih obs wrong: total=%.4f/%.4f overhead=%.1f source=%.4f/%.4f",
                 obs[FC_OBS_PLAYER_PRAYER_LOST], expected_total,
                 obs[FC_OBS_PLAYER_OVERHEAD_PRAYER_LOST],
                 obs[FC_OBS_NPC_START + FC_NPC_PRAYER_DRAIN_DEALT],
                 expected_source);
        fc_destroy(&state);
        return fail(msg);
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    state.npcs_remaining = 1;
    state.player.prayer = PRAYER_PROTECT_MELEE;
    state.player.prayer_drain_counter = 60 + 2 * state.player.prayer_bonus;
    fc_step(&state, actions);
    fc_write_obs(&state, obs);
    expected_total = 10.0f / (float)state.player.max_prayer;
    if (fabsf(obs[FC_OBS_PLAYER_PRAYER_LOST] - expected_total) > eps ||
        obs[FC_OBS_PLAYER_OVERHEAD_PRAYER_LOST] != 1.0f) {
        fc_destroy(&state);
        return fail("passive Prayer loss was not separated from NPC drain");
    }
    fc_step(&state, actions);
    fc_write_obs(&state, obs);
    if (obs[FC_OBS_PLAYER_PRAYER_LOST] != 0.0f ||
        obs[FC_OBS_PLAYER_OVERHEAD_PRAYER_LOST] != 0.0f) {
        fc_destroy(&state);
        return fail("Prayer loss event observations did not clear next tick");
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_MEJKOT, 11, 10, 0);
    state.npcs_remaining = 1;
    state.npcs[0].current_hp = state.npcs[0].max_hp / 4;
    state.npcs[0].attack_timer = 0;
    state.progress_required_work_start = (float)state.npcs[0].max_hp;
    fc_npc_tick(&state, 0);
    fc_write_obs(&state, obs);

    int base = FC_OBS_NPC_START;
    float expected_received = (float)state.npcs[0].heal_amount /
                              (float)state.npcs[0].max_hp;
    if (fabsf(obs[base + FC_NPC_HEAL_RECEIVED] - expected_received) > eps ||
        fabsf(obs[base + FC_NPC_HEAL_GIVEN] - 1.0f) > eps ||
        obs[base + FC_NPC_HEALED_BY_MEJKOT] != 1.0f ||
        obs[base + FC_NPC_HEALED_BY_HURKOT] != 0.0f ||
        obs[base + FC_NPC_HEALED_SELF] != 1.0f ||
        obs[base + FC_NPC_TARGETS_PLAYER] != 1.0f ||
        fabsf(obs[base + FC_NPC_HEAL_COOLDOWN] - 1.0f) > eps ||
        fabsf(obs[FC_OBS_META_START + FC_OBS_META_NPC_HEALING] -
              expected_received) > eps) {
        fc_destroy(&state);
        return fail("Yt-MejKot source, target, self-heal, cooldown, or total-heal obs wrong");
    }

    fc_step(&state, actions);
    fc_write_obs(&state, obs);
    if (obs[base + FC_NPC_HEAL_RECEIVED] != 0.0f ||
        obs[base + FC_NPC_HEAL_GIVEN] != 0.0f ||
        obs[base + FC_NPC_HEALED_BY_MEJKOT] != 0.0f ||
        obs[base + FC_NPC_HEALED_SELF] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_NPC_HEALING] != 0.0f) {
        fc_destroy(&state);
        return fail("healing event observations did not clear next tick");
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_MEJKOT, 11, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_TOK_XIL, 15, 10, 1);
    state.npcs_remaining = 2;
    state.npcs[0].attack_timer = 0;
    state.npcs[1].current_hp = state.npcs[1].max_hp / 4;
    fc_npc_tick(&state, 0);
    fc_write_obs(&state, obs);
    int source_slot = observation_slot_for_npc(&state, 0);
    int target_slot = observation_slot_for_npc(&state, 1);
    if (source_slot < 0 || target_slot < 0) {
        fc_destroy(&state);
        return fail("MejKot healing source or target was not visible");
    }
    int source_base = FC_OBS_NPC_START + source_slot * FC_OBS_NPC_STRIDE;
    int target_base = FC_OBS_NPC_START + target_slot * FC_OBS_NPC_STRIDE;
    expected_received = (float)state.npcs[0].heal_amount /
                        (float)state.npcs[1].max_hp;
    if (obs[source_base + FC_NPC_HEAL_GIVEN] != 1.0f ||
        obs[source_base + FC_NPC_HEALED_SELF] != 0.0f ||
        fabsf(obs[target_base + FC_NPC_HEAL_RECEIVED] - expected_received) > eps ||
        obs[target_base + FC_NPC_HEALED_BY_MEJKOT] != 1.0f ||
        obs[target_base + FC_NPC_HEALED_SELF] != 0.0f) {
        fc_destroy(&state);
        return fail("MejKot-to-other source and recipient observations are not paired");
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 12, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_YT_HURKOT, 11, 10, 1);
    state.npcs_remaining = 2;
    state.npcs[0].current_hp = 1000;
    state.npcs[1].heal_timer = 1;
    state.progress_required_work_start = (float)state.npcs[0].max_hp;
    fc_npc_tick(&state, 1);
    fc_write_obs(&state, obs);
    source_slot = observation_slot_for_npc(&state, 1);
    target_slot = observation_slot_for_npc(&state, 0);
    if (source_slot < 0 || target_slot < 0) {
        fc_destroy(&state);
        return fail("HurKot healing source or Jad target was not visible");
    }
    source_base = FC_OBS_NPC_START + source_slot * FC_OBS_NPC_STRIDE;
    target_base = FC_OBS_NPC_START + target_slot * FC_OBS_NPC_STRIDE;
    expected_received = (float)state.npcs[1].heal_amount /
                        (float)state.npcs[0].max_hp;
    if (obs[source_base + FC_NPC_HEAL_GIVEN] != 1.0f ||
        obs[source_base + FC_NPC_TARGETS_PLAYER] != 0.0f ||
        fabsf(obs[source_base + FC_NPC_HEAL_COOLDOWN] - 1.0f) > eps ||
        fabsf(obs[target_base + FC_NPC_HEAL_RECEIVED] - expected_received) > eps ||
        obs[target_base + FC_NPC_HEALED_BY_HURKOT] != 1.0f ||
        obs[target_base + FC_NPC_HEALED_BY_MEJKOT] != 0.0f) {
        fc_destroy(&state);
        return fail("untagged HurKot source, Jad recipient, or cooldown observations are wrong");
    }
    fc_destroy(&state);

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_YT_HURKOT, 11, 10, 0);
    fc_queue_pending_hit(state.npcs[0].pending_hits,
                         &state.npcs[0].num_pending_hits,
                         FC_MAX_PENDING_HITS,
                         0, 1, ATTACK_RANGED, -1, 0);
    fc_resolve_npc_pending_hits(&state, 0);
    fc_write_obs(&state, obs);
    if (obs[FC_OBS_NPC_START + FC_NPC_TARGETS_PLAYER] != 1.0f ||
        obs[FC_OBS_NPC_START + FC_NPC_HEAL_COOLDOWN] != 0.0f) {
        fc_destroy(&state);
        return fail("tagged HurKot does not expose permanent player targeting");
    }

    fc_destroy(&state);
    return pass("Prayer loss, healing source/target, aggro, and cooldown observations are explicit");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <target_identity|npc_type_obs_one_hot|zero_damage_reward|safespot_reward_disabled|npc_heal_penalty_actual_heal|prayer_loss_penalty|correct_prayer_reward_all_npcs|no_attack_penalty_wave_scaled|player_death_progress_scaled|simple_reward_zeroed_channels|npc_kill_reward_eligibility|tz_kek_split_kill_rewards|wave_clear_reward_scaling|net_progress_required_work|net_progress_wave_clear|net_progress_tz_kek|net_progress_jad|net_progress_timer_clip|progress_observation_fields|prayer_deadline_observation_fields|healer_spawn_validity|special_tz_kih_prayer_drain|special_mejkot_heal_replaces_attack|special_adjacent_style_selection|special_hurkot_behavior|mechanics_observation_events|safespot_los|diagonal_corner_clipping|npc_moves_when_attack_blocked|option_b_no_move_into_range_fire_same_tick|option_b_no_move_into_los_fire_same_tick|option_b_attack_then_move_when_already_valid|option_b_queued_projectile_survives_later_movement|step1_movement_only_clears_stale_target|step1_projectile_survives_movement_cancel|step1_attack_move_clears_continued_target|step1_attack_move_out_of_range_clears_target|step1_directional_cancels_old_approach_route|step1_directional_beats_old_tile_route|step1_attack_approach_without_explicit_movement|step2_occupancy_marks_and_ignores_entities|step2_dynamic_footprint_static_and_occupied|step2_entity_wrapper_ignores_self_blocks_others|step2_dynamic_diagonal_blocks_occupied_corner|step2_dynamic_bfs_avoids_occupied_steps|step2_dynamic_sized_bfs_checks_full_footprint|step2_start_reservation_blocks_swap_tile|step3_player_directional_blocked_by_npc|step3_player_tile_route_rejects_occupied_target|step3_npc_blocked_by_other_npc|step3_large_npc_blocked_by_healer_footprint|step3_tz_kek_split_avoids_occupied_tiles|step4_ranged_npc_chases_player_bounds_not_los_tile|step4_large_npc_chase_checks_full_footprint|step4_npc_stays_when_current_position_can_attack|invalid_action_classes|hp_regeneration_interval>\n",
                argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "invalid_action_classes") == 0) return test_invalid_action_classes();
    if (strcmp(argv[1], "hp_regeneration_interval") == 0) return test_hp_regeneration_interval();
    if (strcmp(argv[1], "target_identity") == 0) return test_target_identity();
    if (strcmp(argv[1], "npc_type_obs_one_hot") == 0) return test_npc_type_obs_one_hot();
    if (strcmp(argv[1], "zero_damage_reward") == 0) return test_zero_damage_reward();
    if (strcmp(argv[1], "safespot_reward_disabled") == 0) return test_safespot_reward_disabled();
    if (strcmp(argv[1], "npc_heal_penalty_actual_heal") == 0) return test_npc_heal_penalty_actual_heal();
    if (strcmp(argv[1], "prayer_loss_penalty") == 0) return test_prayer_loss_penalty();
    if (strcmp(argv[1], "correct_prayer_reward_all_npcs") == 0) return test_correct_prayer_reward_all_npcs();
    if (strcmp(argv[1], "no_attack_penalty_wave_scaled") == 0) return test_no_attack_penalty_wave_scaled();
    if (strcmp(argv[1], "player_death_progress_scaled") == 0) return test_player_death_progress_scaled();
    if (strcmp(argv[1], "simple_reward_zeroed_channels") == 0) return test_simple_reward_zeroed_channels();
    if (strcmp(argv[1], "npc_kill_reward_eligibility") == 0) return test_npc_kill_reward_eligibility();
    if (strcmp(argv[1], "tz_kek_split_kill_rewards") == 0) return test_tz_kek_split_kill_rewards();
    if (strcmp(argv[1], "wave_clear_reward_scaling") == 0) return test_wave_clear_reward_scaling();
    if (strcmp(argv[1], "net_progress_required_work") == 0) return test_net_progress_required_work();
    if (strcmp(argv[1], "net_progress_wave_clear") == 0) return test_net_progress_wave_clear_transition();
    if (strcmp(argv[1], "net_progress_tz_kek") == 0) return test_net_progress_tz_kek_accounting();
    if (strcmp(argv[1], "net_progress_jad") == 0) return test_net_progress_jad_accounting();
    if (strcmp(argv[1], "net_progress_timer_clip") == 0) return test_net_progress_timer_and_clip_sanity();
    if (strcmp(argv[1], "progress_observation_fields") == 0) return test_progress_observation_fields();
    if (strcmp(argv[1], "prayer_deadline_observation_fields") == 0) return test_prayer_deadline_observation_fields();
    if (strcmp(argv[1], "healer_spawn_validity") == 0) return test_healer_spawn_validity();
    if (strcmp(argv[1], "special_tz_kih_prayer_drain") == 0) return test_special_tz_kih_prayer_drain();
    if (strcmp(argv[1], "special_mejkot_heal_replaces_attack") == 0) return test_special_mejkot_heal_replaces_attack();
    if (strcmp(argv[1], "special_adjacent_style_selection") == 0) return test_special_adjacent_style_selection();
    if (strcmp(argv[1], "special_hurkot_behavior") == 0) return test_special_hurkot_behavior();
    if (strcmp(argv[1], "mechanics_observation_events") == 0) return test_mechanics_observation_events();
    if (strcmp(argv[1], "safespot_los") == 0) return test_safespot_los();
    if (strcmp(argv[1], "diagonal_corner_clipping") == 0) return test_diagonal_corner_clipping();
    if (strcmp(argv[1], "npc_moves_when_attack_blocked") == 0) return test_npc_moves_when_attack_blocked();
    if (strcmp(argv[1], "option_b_no_move_into_range_fire_same_tick") == 0) return test_option_b_no_move_into_range_fire_same_tick();
    if (strcmp(argv[1], "option_b_no_move_into_los_fire_same_tick") == 0) return test_option_b_no_move_into_los_fire_same_tick();
    if (strcmp(argv[1], "option_b_attack_then_move_when_already_valid") == 0) return test_option_b_attack_then_move_when_already_valid();
    if (strcmp(argv[1], "option_b_queued_projectile_survives_later_movement") == 0) return test_option_b_queued_projectile_survives_later_movement();
    if (strcmp(argv[1], "step1_movement_only_clears_stale_target") == 0) return test_step1_movement_only_clears_stale_target();
    if (strcmp(argv[1], "step1_projectile_survives_movement_cancel") == 0) return test_step1_projectile_survives_movement_cancel();
    if (strcmp(argv[1], "step1_attack_move_clears_continued_target") == 0) return test_step1_attack_move_clears_continued_target();
    if (strcmp(argv[1], "step1_attack_move_out_of_range_clears_target") == 0) return test_step1_attack_move_out_of_range_clears_target();
    if (strcmp(argv[1], "step1_directional_cancels_old_approach_route") == 0) return test_step1_directional_cancels_old_approach_route();
    if (strcmp(argv[1], "step1_directional_beats_old_tile_route") == 0) return test_step1_directional_beats_old_tile_route();
    if (strcmp(argv[1], "step1_attack_approach_without_explicit_movement") == 0) return test_step1_attack_approach_without_explicit_movement();
    if (strcmp(argv[1], "step2_occupancy_marks_and_ignores_entities") == 0) return test_step2_occupancy_marks_and_ignores_entities();
    if (strcmp(argv[1], "step2_dynamic_footprint_static_and_occupied") == 0) return test_step2_dynamic_footprint_static_and_occupied();
    if (strcmp(argv[1], "step2_entity_wrapper_ignores_self_blocks_others") == 0) return test_step2_entity_wrapper_ignores_self_blocks_others();
    if (strcmp(argv[1], "step2_dynamic_diagonal_blocks_occupied_corner") == 0) return test_step2_dynamic_diagonal_blocks_occupied_corner();
    if (strcmp(argv[1], "step2_dynamic_bfs_avoids_occupied_steps") == 0) return test_step2_dynamic_bfs_avoids_occupied_steps();
    if (strcmp(argv[1], "step2_dynamic_sized_bfs_checks_full_footprint") == 0) return test_step2_dynamic_sized_bfs_checks_full_footprint();
    if (strcmp(argv[1], "step2_start_reservation_blocks_swap_tile") == 0) return test_step2_start_reservation_blocks_swap_tile();
    if (strcmp(argv[1], "step3_player_directional_blocked_by_npc") == 0) return test_step3_player_directional_blocked_by_npc();
    if (strcmp(argv[1], "step3_player_tile_route_rejects_occupied_target") == 0) return test_step3_player_tile_route_rejects_occupied_target();
    if (strcmp(argv[1], "step3_npc_blocked_by_other_npc") == 0) return test_step3_npc_blocked_by_other_npc();
    if (strcmp(argv[1], "step3_large_npc_blocked_by_healer_footprint") == 0) return test_step3_large_npc_blocked_by_healer_footprint();
    if (strcmp(argv[1], "step3_tz_kek_split_avoids_occupied_tiles") == 0) return test_step3_tz_kek_split_avoids_occupied_tiles();
    if (strcmp(argv[1], "step4_ranged_npc_chases_player_bounds_not_los_tile") == 0) return test_step4_ranged_npc_chases_player_bounds_not_los_tile();
    if (strcmp(argv[1], "step4_large_npc_chase_checks_full_footprint") == 0) return test_step4_large_npc_chase_checks_full_footprint();
    if (strcmp(argv[1], "step4_npc_stays_when_current_position_can_attack") == 0) return test_step4_npc_stays_when_current_position_can_attack();

    fprintf(stderr, "unknown guardrail: %s\n", argv[1]);
    return 2;
}
