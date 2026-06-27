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

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <target_identity|npc_type_obs_one_hot|zero_damage_reward|safespot_reward_disabled|healer_spawn_validity|safespot_los|invalid_action_classes>\n",
                argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "invalid_action_classes") == 0) return test_invalid_action_classes();
    if (strcmp(argv[1], "target_identity") == 0) return test_target_identity();
    if (strcmp(argv[1], "npc_type_obs_one_hot") == 0) return test_npc_type_obs_one_hot();
    if (strcmp(argv[1], "zero_damage_reward") == 0) return test_zero_damage_reward();
    if (strcmp(argv[1], "safespot_reward_disabled") == 0) return test_safespot_reward_disabled();
    if (strcmp(argv[1], "healer_spawn_validity") == 0) return test_healer_spawn_validity();
    if (strcmp(argv[1], "safespot_los") == 0) return test_safespot_los();

    fprintf(stderr, "unknown guardrail: %s\n", argv[1]);
    return 2;
}
