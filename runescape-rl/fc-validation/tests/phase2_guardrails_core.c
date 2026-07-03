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

    if (state.player.attack_target_idx == 0) {
        fc_destroy(&state);
        return pass("attack slot stayed bound to the observed NPC identity");
    }

    char msg[160];
    snprintf(msg, sizeof(msg),
             "attack slot drifted: expected NPC index 0, got %d after movement",
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

static int test_net_progress_required_work(void) {
    FcState state;
    FcRewardParams params;
    FcRewardRuntime runtime;
    FcRewardBreakdown breakdown;

    make_open_manual_state(&state, 10, 10);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 12, 10, 0);
    state.npcs_remaining = 1;

    memset(&params, 0, sizeof(params));
    params.w_progress = 0.001f;
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
        fabsf(breakdown.progress + 0.01f) > 0.0001f ||
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
    return pass("net required-work progress rewards damage and penalizes healing");
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

    make_open_manual_state(&state, 20, 20);
    state.current_wave = FC_NUM_WAVES;
    fc_npc_spawn(&state.npcs[0], NPC_TZTOK_JAD, 10, 10, 0);
    state.npcs[0].current_hp = FC_JAD_HEALER_THRESHOLD_HP_TENTHS - 10;
    state.npcs_remaining = 1;
    state.next_spawn_index = 1;
    state.walkable[8][10] = 0;  /* first preferred healer tile */

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
    }

    if (healer_count == FC_JAD_NUM_HEALERS) {
        fc_destroy(&state);
        return pass("Jad healers spawned only on valid walkable footprints");
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "expected %d healers, found %d",
             FC_JAD_NUM_HEALERS, healer_count);
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

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <target_identity|npc_type_obs_one_hot|zero_damage_reward|safespot_reward_disabled|npc_heal_penalty_actual_heal|prayer_loss_penalty|no_attack_penalty_wave_scaled|net_progress_required_work|net_progress_wave_clear|net_progress_tz_kek|net_progress_jad|net_progress_timer_clip|progress_observation_fields|prayer_deadline_observation_fields|healer_spawn_validity|safespot_los|diagonal_corner_clipping|npc_moves_when_attack_blocked|invalid_action_classes>\n",
                argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "invalid_action_classes") == 0) return test_invalid_action_classes();
    if (strcmp(argv[1], "target_identity") == 0) return test_target_identity();
    if (strcmp(argv[1], "npc_type_obs_one_hot") == 0) return test_npc_type_obs_one_hot();
    if (strcmp(argv[1], "zero_damage_reward") == 0) return test_zero_damage_reward();
    if (strcmp(argv[1], "safespot_reward_disabled") == 0) return test_safespot_reward_disabled();
    if (strcmp(argv[1], "npc_heal_penalty_actual_heal") == 0) return test_npc_heal_penalty_actual_heal();
    if (strcmp(argv[1], "prayer_loss_penalty") == 0) return test_prayer_loss_penalty();
    if (strcmp(argv[1], "no_attack_penalty_wave_scaled") == 0) return test_no_attack_penalty_wave_scaled();
    if (strcmp(argv[1], "net_progress_required_work") == 0) return test_net_progress_required_work();
    if (strcmp(argv[1], "net_progress_wave_clear") == 0) return test_net_progress_wave_clear_transition();
    if (strcmp(argv[1], "net_progress_tz_kek") == 0) return test_net_progress_tz_kek_accounting();
    if (strcmp(argv[1], "net_progress_jad") == 0) return test_net_progress_jad_accounting();
    if (strcmp(argv[1], "net_progress_timer_clip") == 0) return test_net_progress_timer_and_clip_sanity();
    if (strcmp(argv[1], "progress_observation_fields") == 0) return test_progress_observation_fields();
    if (strcmp(argv[1], "prayer_deadline_observation_fields") == 0) return test_prayer_deadline_observation_fields();
    if (strcmp(argv[1], "healer_spawn_validity") == 0) return test_healer_spawn_validity();
    if (strcmp(argv[1], "safespot_los") == 0) return test_safespot_los();
    if (strcmp(argv[1], "diagonal_corner_clipping") == 0) return test_diagonal_corner_clipping();
    if (strcmp(argv[1], "npc_moves_when_attack_blocked") == 0) return test_npc_moves_when_attack_blocked();

    fprintf(stderr, "unknown guardrail: %s\n", argv[1]);
    return 2;
}
