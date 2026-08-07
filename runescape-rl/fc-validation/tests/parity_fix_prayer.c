#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_prayer.h"
#include "fc_reward.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void neutral_actions(int actions[FC_NUM_ACTION_HEADS], int prayer,
                            int drink) {
    memset(actions, 0, sizeof(int) * FC_NUM_ACTION_HEADS);
    actions[0] = FC_MOVE_IDLE;
    actions[1] = FC_ATTACK_NONE;
    actions[2] = prayer;
    actions[3] = FC_EAT_NONE;
    actions[4] = drink;
    actions[5] = FC_MOVE_TARGET_X_NONE;
    actions[6] = FC_MOVE_TARGET_Y_NONE;
}

static void make_open_state(FcState* state) {
    fc_init(state);
    fc_reset(state, 123u);
    memset(state->npcs, 0, sizeof(state->npcs));
    memset(state->walkable, 1, sizeof(state->walkable));
    state->terminal = TERMINAL_NONE;
    state->current_wave = 1;
    /* Prevent wave advancement without introducing an NPC into unit cases. */
    state->npcs_remaining = 1;
    state->next_spawn_index = 1;
    state->player.x = 10;
    state->player.y = 10;
    state->player.attack_target_idx = -1;
    state->player.route_len = 0;
    state->player.route_idx = 0;
    state->player.num_pending_hits = 0;
}

static void make_npc_state(FcState* state, int npc_type, int npc_x) {
    make_open_state(state);
    fc_npc_spawn(&state->npcs[0], npc_type, npc_x, 10, 0);
    state->npcs[0].movement_speed = 0;
    state->npcs[0].attack_timer = 0;
}

static int transition_equal(const FcPrayerTransition* actual,
                            const FcPrayerTransition* expected) {
    return actual->prior_prayer == expected->prior_prayer &&
           actual->requested_final_prayer == expected->requested_final_prayer &&
           actual->actual_final_prayer == expected->actual_final_prayer &&
           actual->off_requested == expected->off_requested &&
           actual->off_performed == expected->off_performed &&
           actual->on_requested == expected->on_requested &&
           actual->on_succeeded == expected->on_succeeded &&
           actual->explicit_off_then_on == expected->explicit_off_then_on &&
           actual->final_state_changed == expected->final_state_changed;
}

static int test_pray_001(void) {
    if (FC_PRAYER_NO_CHANGE != 0 || FC_PRAYER_OFF != 1 ||
        FC_PRAYER_MAGIC != 2 || FC_PRAYER_RANGE != 3 ||
        FC_PRAYER_MELEE != 4 || FC_PRAYER_FLICK_MAGIC != 5 ||
        FC_PRAYER_FLICK_RANGE != 6 || FC_PRAYER_FLICK_MELEE != 7) {
        fprintf(stderr, "FAIL PRAY-001: prayer command IDs changed\n");
        return 1;
    }

    FcState state;
    make_open_state(&state);
    int actions[FC_NUM_ACTION_HEADS];
    int invalid[FC_INVALID_ACTION_CLASS_COUNT];
    neutral_actions(actions, -1, FC_DRINK_NONE);
    fc_action_invalid_classes(&state, actions, invalid);
    if (!invalid[FC_INVALID_ACTION_PRAYER]) {
        fprintf(stderr, "FAIL PRAY-001: prayer action -1 was accepted\n");
        return 1;
    }
    neutral_actions(actions, 8, FC_DRINK_NONE);
    fc_action_invalid_classes(&state, actions, invalid);
    if (!invalid[FC_INVALID_ACTION_PRAYER]) {
        fprintf(stderr, "FAIL PRAY-001: prayer action 8 was accepted\n");
        return 1;
    }

    struct {
        float before;
        float mask[FC_ACTION_MASK_SIZE];
        float after;
    } guarded = {12345.0f, {0.0f}, 67890.0f};
    fc_write_mask(&state, guarded.mask);
    if (guarded.before != 12345.0f || guarded.after != 67890.0f) {
        fprintf(stderr, "FAIL PRAY-001: mask writer crossed its declared buffer\n");
        return 1;
    }
    for (int i = 0; i < FC_PRAYER_DIM; i++) {
        if (guarded.mask[FC_MASK_PRAYER_START + i] != 1.0f) {
            fprintf(stderr, "FAIL PRAY-001: prayer action %d is unexpectedly masked\n", i);
            return 1;
        }
    }
    if (FC_PRAYER_DIM != 8) {
        fprintf(stderr,
                "FAIL PRAY-001: FC_PRAYER_DIM=%d, expected backward-compatible dimension 8\n",
                FC_PRAYER_DIM);
        return 1;
    }

    printf("PASS PRAY-001: exact prayer IDs, validation, and mask canaries\n");
    return 0;
}

typedef struct {
    const char* label;
    int prior;
    int points;
    int command;
    FcPrayerTransition expected;
} TransitionCase;

static int test_pray_002(void) {
    static const TransitionCase cases[] = {
        {"off/no-change", PRAYER_NONE, 100, FC_PRAYER_NO_CHANGE,
         {PRAYER_NONE, PRAYER_NONE, PRAYER_NONE, 0, 0, 0, 0, 0, 0}},
        {"off/off", PRAYER_NONE, 100, FC_PRAYER_OFF,
         {PRAYER_NONE, PRAYER_NONE, PRAYER_NONE, 0, 0, 0, 0, 0, 0}},
        {"off/select-magic", PRAYER_NONE, 100, FC_PRAYER_MAGIC,
         {PRAYER_NONE, PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC,
          0, 0, 1, 1, 0, 1}},
        {"off/flick-magic", PRAYER_NONE, 100, FC_PRAYER_FLICK_MAGIC,
         {PRAYER_NONE, PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC,
          1, 0, 1, 1, 1, 1}},
        {"magic/no-change", PRAYER_PROTECT_MAGIC, 100, FC_PRAYER_NO_CHANGE,
         {PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC,
          0, 0, 0, 0, 0, 0}},
        {"magic/select-magic", PRAYER_PROTECT_MAGIC, 100, FC_PRAYER_MAGIC,
         {PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC,
          0, 0, 0, 0, 0, 0}},
        {"magic/off", PRAYER_PROTECT_MAGIC, 100, FC_PRAYER_OFF,
         {PRAYER_PROTECT_MAGIC, PRAYER_NONE, PRAYER_NONE,
          1, 1, 0, 0, 0, 1}},
        {"magic/select-range", PRAYER_PROTECT_MAGIC, 100, FC_PRAYER_RANGE,
         {PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_RANGE, PRAYER_PROTECT_RANGE,
          0, 0, 1, 1, 0, 1}},
        {"magic/flick-magic", PRAYER_PROTECT_MAGIC, 100,
         FC_PRAYER_FLICK_MAGIC,
         {PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC,
          1, 1, 1, 1, 1, 0}},
        {"magic/flick-range", PRAYER_PROTECT_MAGIC, 100,
         FC_PRAYER_FLICK_RANGE,
         {PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_RANGE, PRAYER_PROTECT_RANGE,
          1, 1, 1, 1, 1, 1}},
        {"zero/select-magic", PRAYER_NONE, 0, FC_PRAYER_MAGIC,
         {PRAYER_NONE, PRAYER_PROTECT_MAGIC, PRAYER_NONE,
          0, 0, 1, 0, 0, 0}},
        {"zero/flick-magic", PRAYER_NONE, 0, FC_PRAYER_FLICK_MAGIC,
         {PRAYER_NONE, PRAYER_PROTECT_MAGIC, PRAYER_NONE,
          1, 0, 1, 0, 1, 0}},
        {"zero-active/flick-magic", PRAYER_PROTECT_MAGIC, 0,
         FC_PRAYER_FLICK_MAGIC,
         {PRAYER_PROTECT_MAGIC, PRAYER_PROTECT_MAGIC, PRAYER_NONE,
          1, 1, 1, 0, 1, 1}},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        FcPlayer player;
        memset(&player, 0, sizeof(player));
        player.prayer = cases[i].prior;
        player.current_prayer = cases[i].points;
        FcPrayerTransition actual =
            fc_prayer_apply_action(&player, cases[i].command);
        if (!transition_equal(&actual, &cases[i].expected) ||
            player.prayer != cases[i].expected.actual_final_prayer) {
            fprintf(stderr,
                    "FAIL PRAY-002: %s transition was prior/requested/actual=%d/%d/%d edges=%d/%d/%d/%d explicit=%d changed=%d\n",
                    cases[i].label, actual.prior_prayer,
                    actual.requested_final_prayer, actual.actual_final_prayer,
                    actual.off_requested, actual.off_performed,
                    actual.on_requested, actual.on_succeeded,
                    actual.explicit_off_then_on, actual.final_state_changed);
            return 1;
        }
    }

    FcState state;
    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 100;
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, FC_PRAYER_FLICK_MAGIC, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.prayer != PRAYER_PROTECT_MAGIC ||
        !state.player.prayer_changed_this_tick ||
        state.ep_prayer_switches != 1) {
        fprintf(stderr,
                "FAIL PRAY-002: same-prayer flick final/flag/metric=%d/%d/%d, expected 3/1/1\n",
                state.player.prayer, state.player.prayer_changed_this_tick,
                state.ep_prayer_switches);
        return 1;
    }

    printf("PASS PRAY-002: complete transition-result truth table and metrics\n");
    return 0;
}

static void make_prayer_player(FcPlayer* player, int prayer, int points,
                               int counter, int bonus) {
    memset(player, 0, sizeof(*player));
    player->prayer = prayer;
    player->current_prayer = points;
    player->max_prayer = points > 0 ? points : 100;
    player->prayer_bonus = bonus;
    player->prayer_drain_counter = counter;
}

static int apply_and_drain(FcPlayer* player, int command) {
    int start = player->prayer;
    FcPrayerTransition transition = fc_prayer_apply_action(player, command);
    return fc_prayer_drain_tick(player, start, &transition);
}

static int test_pray_003(void) {
    FcPlayer player;
    make_prayer_player(&player, PRAYER_NONE, 100, 7, 0);
    apply_and_drain(&player, FC_PRAYER_MAGIC);
    if (player.prayer != PRAYER_PROTECT_MAGIC ||
        player.prayer_drain_counter != 7) {
        fprintf(stderr, "FAIL PRAY-003: initial activation was not free\n");
        return 1;
    }

    apply_and_drain(&player, FC_PRAYER_NO_CHANGE);
    if (player.prayer_drain_counter != 19) {
        fprintf(stderr, "FAIL PRAY-003: uninterrupted prayer did not add 12\n");
        return 1;
    }
    apply_and_drain(&player, FC_PRAYER_MAGIC);
    if (player.prayer_drain_counter != 31) {
        fprintf(stderr, "FAIL PRAY-003: same-prayer selection did not add 12\n");
        return 1;
    }
    apply_and_drain(&player, FC_PRAYER_OFF);
    if (player.prayer_drain_counter != 31) {
        fprintf(stderr, "FAIL PRAY-003: OFF did not preserve the counter\n");
        return 1;
    }

    /* Trusted executable reference oracle:
     * PrayerDrain.kt SHA-256
     * 3457d005236f9de1365e7c332971e9abdd8b3d89b599a2e3e93b2b023053e69b
     * FightCaveBackendActionAdapter.kt SHA-256
     * ceb0cd8b428796792a13f6f7993fb59fb7aa2fb10d08adc538ebae9223eb397a
     * Golden Magic->Range sequence: before action 24, after action 24,
     * after the corresponding drain tick 36. */
    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 100, 24, 0);
    int start = player.prayer;
    FcPrayerTransition transition =
        fc_prayer_apply_action(&player, FC_PRAYER_RANGE);
    if (player.prayer_drain_counter != 24) {
        fprintf(stderr, "FAIL PRAY-003: direct switch changed counter on action edge\n");
        return 1;
    }
    fc_prayer_drain_tick(&player, start, &transition);
    if (player.prayer != PRAYER_PROTECT_RANGE ||
        player.prayer_drain_counter != 36) {
        fprintf(stderr,
                "FAIL PRAY-003: oracle direct switch ended prayer/counter=%d/%d, expected 2/36\n",
                player.prayer, player.prayer_drain_counter);
        return 1;
    }

    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 100, 24, 0);
    apply_and_drain(&player, FC_PRAYER_FLICK_MAGIC);
    if (player.prayer != PRAYER_PROTECT_MAGIC ||
        player.prayer_drain_counter != 24) {
        fprintf(stderr,
                "FAIL PRAY-003: same-prayer flick ended prayer/counter=%d/%d, expected 3/24\n",
                player.prayer, player.prayer_drain_counter);
        return 1;
    }

    make_prayer_player(&player, PRAYER_NONE, 100, 24, 0);
    apply_and_drain(&player, FC_PRAYER_FLICK_MAGIC);
    if (player.prayer != PRAYER_PROTECT_MAGIC ||
        player.prayer_drain_counter != 24) {
        fprintf(stderr, "FAIL PRAY-003: Off-to-flick activation was not free\n");
        return 1;
    }
    apply_and_drain(&player, FC_PRAYER_FLICK_RANGE);
    if (player.prayer != PRAYER_PROTECT_RANGE ||
        player.prayer_drain_counter != 24) {
        fprintf(stderr, "FAIL PRAY-003: cross-prayer flick was not free\n");
        return 1;
    }

    printf("PASS PRAY-003: complete drain decision table and oracle switch\n");
    return 0;
}

static int test_pray_004(void) {
    static const int bonuses[] = {0, 6, 8, 11};
    for (size_t i = 0; i < sizeof(bonuses) / sizeof(bonuses[0]); i++) {
        int resistance = 60 + 2 * bonuses[i];
        FcPlayer player;
        make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 100,
                           resistance - 12, bonuses[i]);
        apply_and_drain(&player, FC_PRAYER_NO_CHANGE);
        if (player.current_prayer != 100 ||
            player.prayer_drain_counter != resistance) {
            fprintf(stderr,
                    "FAIL PRAY-004: bonus %d equality boundary drained (%d/%d)\n",
                    bonuses[i], player.current_prayer,
                    player.prayer_drain_counter);
            return 1;
        }
        apply_and_drain(&player, FC_PRAYER_NO_CHANGE);
        if (player.current_prayer != 90 ||
            player.prayer_drain_counter != 12) {
            fprintf(stderr,
                    "FAIL PRAY-004: bonus %d strict boundary result=%d/%d, expected 90/12\n",
                    bonuses[i], player.current_prayer,
                    player.prayer_drain_counter);
            return 1;
        }

        make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 100,
                           resistance * 3 + 1, bonuses[i]);
        apply_and_drain(&player, FC_PRAYER_NO_CHANGE);
        if (player.current_prayer != 70 ||
            player.prayer_drain_counter != 13 ||
            player.prayer_drain_counter > resistance) {
            fprintf(stderr,
                    "FAIL PRAY-004: bonus %d multi-drain result=%d/%d\n",
                    bonuses[i], player.current_prayer,
                    player.prayer_drain_counter);
            return 1;
        }
    }

    FcPlayer player;
    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 100, 31, 6);
    apply_and_drain(&player, FC_PRAYER_OFF);
    if (player.prayer_drain_counter != 31) {
        fprintf(stderr, "FAIL PRAY-004: OFF discarded fractional drain\n");
        return 1;
    }
    player.prayer = PRAYER_PROTECT_MAGIC;
    apply_and_drain(&player, FC_PRAYER_RANGE);
    if (player.prayer_drain_counter != 43) {
        fprintf(stderr, "FAIL PRAY-004: direct switch did not advance fraction\n");
        return 1;
    }
    apply_and_drain(&player, FC_PRAYER_FLICK_MAGIC);
    if (player.prayer != PRAYER_PROTECT_MAGIC ||
        player.prayer_drain_counter != 43) {
        fprintf(stderr,
                "FAIL PRAY-004: explicit flick changed preserved fraction to prayer/counter=%d/%d\n",
                player.prayer, player.prayer_drain_counter);
        return 1;
    }

    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 10, 60, 0);
    apply_and_drain(&player, FC_PRAYER_NO_CHANGE);
    if (player.current_prayer != 0 || player.prayer != PRAYER_NONE ||
        player.prayer_drain_counter != 0) {
        fprintf(stderr, "FAIL PRAY-004: depletion did not reset active state\n");
        return 1;
    }

    FcState state;
    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 10;
    state.player.prayer_bonus = 0;
    state.player.prayer_drain_counter = 60;
    FcPrayerTransition transition =
        fc_prayer_apply_action(&state.player, FC_PRAYER_NO_CHANGE);
    fc_prayer_drain_tick(&state.player, PRAYER_PROTECT_MAGIC, &transition);
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_PRAYER_POT);
    fc_tick(&state, actions);
    if (state.player.current_prayer <= 0 ||
        state.player.prayer != PRAYER_NONE ||
        state.player.prayer_drain_counter != 0) {
        fprintf(stderr,
                "FAIL PRAY-004: potion after depletion did not restart from zero\n");
        return 1;
    }

    make_open_state(&state);
    state.player.prayer = PRAYER_NONE;
    state.player.current_prayer = state.player.max_prayer - 100;
    state.player.prayer_drain_counter = 41;
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_PRAYER_POT);
    fc_tick(&state, actions);
    if (state.player.prayer_drain_counter != 41) {
        fprintf(stderr, "FAIL PRAY-004: non-depleting potion reset fraction\n");
        return 1;
    }
    state.player.prayer_drain_counter = 41;
    fc_reset(&state, 321u);
    if (state.player.prayer_drain_counter != 0) {
        fprintf(stderr, "FAIL PRAY-004: episode reset retained fraction\n");
        return 1;
    }

    printf("PASS PRAY-004: resistance boundaries and fraction persistence\n");
    return 0;
}

static int test_pray_005(void) {
    FcPlayer player;
    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 10, 37, 0);
    int lost = fc_prayer_apply_loss_tenths(&player, 10);
    if (lost != 10 || player.current_prayer != 0 ||
        player.prayer != PRAYER_NONE || player.prayer_drain_counter != 0) {
        fprintf(stderr,
                "FAIL PRAY-005: central depletion result lost/points/prayer/counter=%d/%d/%d/%d\n",
                lost, player.current_prayer, player.prayer,
                player.prayer_drain_counter);
        return 1;
    }
    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 20, 37, 0);
    lost = fc_prayer_apply_loss_tenths(&player, 10);
    if (lost != 10 || player.current_prayer != 10 ||
        player.prayer != PRAYER_PROTECT_MAGIC ||
        player.prayer_drain_counter != 37) {
        fprintf(stderr, "FAIL PRAY-005: non-depleting central loss reset state\n");
        return 1;
    }

    static const int zero_safe_commands[] = {
        FC_PRAYER_NO_CHANGE, FC_PRAYER_OFF,
        FC_PRAYER_MAGIC, FC_PRAYER_FLICK_MAGIC,
    };
    for (size_t i = 0;
         i < sizeof(zero_safe_commands) / sizeof(zero_safe_commands[0]); i++) {
        make_prayer_player(&player, PRAYER_NONE, 0, 0, 0);
        apply_and_drain(&player, zero_safe_commands[i]);
        if (player.current_prayer != 0 || player.prayer != PRAYER_NONE ||
            player.prayer_drain_counter != 0) {
            fprintf(stderr,
                    "FAIL PRAY-005: zero-Prayer command %d created live state\n",
                    zero_safe_commands[i]);
            return 1;
        }
    }

    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 10, 60, 0);
    apply_and_drain(&player, FC_PRAYER_NO_CHANGE);
    if (player.current_prayer != 0 || player.prayer != PRAYER_NONE ||
        player.prayer_drain_counter != 0) {
        fprintf(stderr, "FAIL PRAY-005: passive depletion invariant diverged\n");
        return 1;
    }
    make_prayer_player(&player, PRAYER_PROTECT_MAGIC, 20, 60, 0);
    apply_and_drain(&player, FC_PRAYER_NO_CHANGE);
    if (player.current_prayer != 10 ||
        player.prayer != PRAYER_PROTECT_MAGIC ||
        player.prayer_drain_counter != 12) {
        fprintf(stderr, "FAIL PRAY-005: passive non-depletion reset state\n");
        return 1;
    }

    FcState state;
    make_npc_state(&state, NPC_TZ_KIH, 11);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 10;
    state.player.prayer_drain_counter = 37;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 0, 1, ATTACK_MELEE, 0,
                         fc_npc_get_stats(NPC_TZ_KIH)->prayer_drain);
    state.player.pending_hits[0].prayer_snapshot = PRAYER_PROTECT_MAGIC;
    fc_resolve_player_pending_hits(&state);
    if (state.player.current_prayer != 0 ||
        state.player.prayer != PRAYER_NONE ||
        state.player.prayer_drain_counter != 0) {
        fprintf(stderr, "FAIL PRAY-005: Tz-Kih depletion invariant diverged\n");
        return 1;
    }

    make_npc_state(&state, NPC_TZ_KIH, 11);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 20;
    state.player.prayer_drain_counter = 37;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 0, 1, ATTACK_MELEE, 0,
                         fc_npc_get_stats(NPC_TZ_KIH)->prayer_drain);
    state.player.pending_hits[0].prayer_snapshot = PRAYER_PROTECT_MAGIC;
    fc_resolve_player_pending_hits(&state);
    if (state.player.current_prayer != 10 ||
        state.player.prayer != PRAYER_PROTECT_MAGIC ||
        state.player.prayer_drain_counter != 37) {
        fprintf(stderr, "FAIL PRAY-005: Tz-Kih non-depletion reset state\n");
        return 1;
    }

    static const int zero_commands[] = {
        FC_PRAYER_MAGIC, FC_PRAYER_FLICK_MAGIC,
    };
    for (size_t i = 0; i < sizeof(zero_commands) / sizeof(zero_commands[0]); i++) {
        make_open_state(&state);
        state.player.prayer = PRAYER_NONE;
        state.player.current_prayer = 0;
        state.player.prayer_drain_counter = 0;
        state.player.prayer_doses_remaining = 1;
        state.player.potion_timer = 0;
        int actions[FC_NUM_ACTION_HEADS];
        neutral_actions(actions, zero_commands[i], FC_DRINK_PRAYER_POT);
        fc_tick(&state, actions);
        if (state.player.current_prayer <= 0 ||
            state.player.prayer != PRAYER_NONE ||
            state.player.prayer_drain_counter != 0) {
            fprintf(stderr,
                    "FAIL PRAY-005: zero-Prayer command %d retroactively activated after potion\n",
                    zero_commands[i]);
            return 1;
        }
    }

    make_open_state(&state);
    state.player.current_prayer = state.player.max_prayer - 200;
    state.player.prayer = PRAYER_NONE;
    state.player.prayer_drain_counter = 37;
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_PRAYER_POT);
    fc_tick(&state, actions);
    if (state.player.prayer_drain_counter != 37) {
        fprintf(stderr, "FAIL PRAY-005: potion without depletion reset fraction\n");
        return 1;
    }

    make_open_state(&state);
    fc_npc_spawn(&state.npcs[0], NPC_TZ_KIH, 11, 10, 0);
    fc_npc_spawn(&state.npcs[1], NPC_TZ_KEK, 12, 10, 1);
    state.npcs_remaining = 2;
    state.player.prayer = PRAYER_PROTECT_MELEE;
    state.player.prayer_at_tick_start = PRAYER_PROTECT_MELEE;
    state.player.current_prayer = 10;
    state.player.prayer_drain_counter = 37;
    int hp_before = state.player.current_hp;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 0, 1, ATTACK_MELEE, 0,
                         fc_npc_get_stats(NPC_TZ_KIH)->prayer_drain);
    state.player.pending_hits[0].prayer_snapshot = PRAYER_PROTECT_MELEE;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 100, 1, ATTACK_MELEE, 1, 0);
    state.player.pending_hits[1].prayer_snapshot = PRAYER_PROTECT_MELEE;
    fc_resolve_player_pending_hits(&state);
    if (state.player.current_prayer != 0 ||
        state.player.prayer != PRAYER_NONE ||
        state.player.prayer_drain_counter != 0 ||
        state.player.current_hp != hp_before ||
        !state.player.hit_blocked_this_tick) {
        fprintf(stderr,
                "FAIL PRAY-005: Tz-Kih-first ordered hits lost immutable protection\n");
        return 1;
    }

    make_npc_state(&state, NPC_KET_ZEK, 20);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 10;
    state.player.prayer_bonus = 0;
    state.player.prayer_drain_counter = 60;
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.current_prayer != 0 ||
        state.player.prayer != PRAYER_NONE ||
        state.player.prayer_drain_counter != 0 ||
        state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].attack_style != ATTACK_MAGIC ||
        state.player.pending_hits[0].prayer_snapshot != PRAYER_PROTECT_MAGIC) {
        fprintf(stderr,
                "FAIL PRAY-005: passive-first snapshot points/prayer/counter/hits/snapshot=%d/%d/%d/%d/%d\n",
                state.player.current_prayer, state.player.prayer,
                state.player.prayer_drain_counter,
                state.player.num_pending_hits,
                state.player.num_pending_hits > 0
                    ? state.player.pending_hits[0].prayer_snapshot : -99);
        return 1;
    }
    state.player.pending_hits[0].damage = 100;
    state.player.pending_hits[0].ticks_remaining = 1;
    hp_before = state.player.current_hp;
    fc_resolve_player_pending_hits(&state);
    if (state.player.current_hp != hp_before ||
        !state.player.hit_blocked_this_tick) {
        fprintf(stderr, "FAIL PRAY-005: passive-first attack was not blocked\n");
        return 1;
    }

    printf("PASS PRAY-005: centralized depletion and zero-Prayer ordering\n");
    return 0;
}

static int run_ordinary_snapshot_case(int start_prayer, int command,
                                      int expected_snapshot,
                                      int expected_final, int expected_counter,
                                      int expect_blocked) {
    FcState state;
    make_npc_state(&state, NPC_KET_ZEK, 20);
    state.player.prayer = start_prayer;
    state.player.current_prayer = 1000;
    state.player.max_prayer = 1000;
    state.player.prayer_drain_counter = expected_counter;
    if (start_prayer != PRAYER_NONE &&
        expected_final != PRAYER_NONE &&
        command != FC_PRAYER_FLICK_MAGIC &&
        command != FC_PRAYER_FLICK_RANGE &&
        command != FC_PRAYER_FLICK_MELEE) {
        state.player.prayer_drain_counter -= 12;
    }
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, command, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].attack_style != ATTACK_MAGIC ||
        state.player.pending_hits[0].prayer_snapshot != expected_snapshot ||
        state.player.prayer != expected_final ||
        state.player.prayer_drain_counter != expected_counter) {
        fprintf(stderr,
                "FAIL PRAY-006: start/cmd=%d/%d produced hits/style/snapshot/final/counter=%d/%d/%d/%d/%d\n",
                start_prayer, command, state.player.num_pending_hits,
                state.player.num_pending_hits > 0
                    ? state.player.pending_hits[0].attack_style : -99,
                state.player.num_pending_hits > 0
                    ? state.player.pending_hits[0].prayer_snapshot : -99,
                state.player.prayer, state.player.prayer_drain_counter);
        return 1;
    }

    int immutable = state.player.pending_hits[0].prayer_snapshot;
    state.player.prayer = immutable == PRAYER_PROTECT_MAGIC
        ? PRAYER_NONE : PRAYER_PROTECT_MAGIC;
    if (state.player.pending_hits[0].prayer_snapshot != immutable) {
        fprintf(stderr, "FAIL PRAY-006: live prayer mutation rewrote queued snapshot\n");
        return 1;
    }
    state.player.pending_hits[0].damage = 100;
    state.player.pending_hits[0].ticks_remaining = 1;
    int hp_before = state.player.current_hp;
    fc_resolve_player_pending_hits(&state);
    int blocked = state.player.current_hp == hp_before;
    if (blocked != expect_blocked) {
        fprintf(stderr,
                "FAIL PRAY-006: snapshot %d blocked=%d, expected %d\n",
                immutable, blocked, expect_blocked);
        return 1;
    }
    return 0;
}

static int test_pray_006(void) {
    if (run_ordinary_snapshot_case(PRAYER_NONE, FC_PRAYER_MAGIC,
                                   PRAYER_NONE, PRAYER_PROTECT_MAGIC, 0, 0) ||
        run_ordinary_snapshot_case(PRAYER_PROTECT_MAGIC, FC_PRAYER_OFF,
                                   PRAYER_PROTECT_MAGIC, PRAYER_NONE, 0, 1) ||
        run_ordinary_snapshot_case(PRAYER_PROTECT_MAGIC, FC_PRAYER_FLICK_MAGIC,
                                   PRAYER_PROTECT_MAGIC,
                                   PRAYER_PROTECT_MAGIC, 31, 1) ||
        run_ordinary_snapshot_case(PRAYER_PROTECT_MAGIC, FC_PRAYER_FLICK_RANGE,
                                   PRAYER_PROTECT_MAGIC,
                                   PRAYER_PROTECT_RANGE, 31, 1)) {
        return 1;
    }

    FcState state;
    make_npc_state(&state, NPC_KET_ZEK, 20);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 1000;
    state.player.max_prayer = 1000;
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, FC_PRAYER_RANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].prayer_snapshot != PRAYER_PROTECT_MAGIC ||
        state.player.prayer != PRAYER_PROTECT_RANGE) {
        fprintf(stderr, "FAIL PRAY-006: direct switch did not use start Magic\n");
        return 1;
    }
    state.npcs[0].attack_timer = 0;
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 2 ||
        state.player.pending_hits[1].prayer_snapshot != PRAYER_PROTECT_RANGE) {
        fprintf(stderr, "FAIL PRAY-006: next-tick attack did not use Range\n");
        return 1;
    }


    make_npc_state(&state, NPC_KET_ZEK, 20);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 1000;
    state.player.max_prayer = 1000;
    state.player.prayer_drain_counter = 31;
    neutral_actions(actions, FC_PRAYER_FLICK_RANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].prayer_snapshot != PRAYER_PROTECT_MAGIC ||
        state.player.prayer != PRAYER_PROTECT_RANGE ||
        state.player.prayer_drain_counter != 31) {
        fprintf(stderr, "FAIL PRAY-006: cross-prayer flick current-tick contract mismatch\n");
        return 1;
    }
    state.npcs[0].attack_timer = 0;
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 2 ||
        state.player.pending_hits[1].prayer_snapshot != PRAYER_PROTECT_RANGE) {
        fprintf(stderr, "FAIL PRAY-006: post-flick next tick did not use Range\n");
        return 1;
    }

    printf("PASS PRAY-006: ordinary attacks use immutable tick-start prayer\n");
    return 0;
}

static int run_flick_trace(int initial_counter, int flick) {
    FcState state;
    make_npc_state(&state, NPC_TZ_KEK, 11);
    state.player.prayer = PRAYER_PROTECT_MELEE;
    state.player.current_prayer = 100000;
    state.player.max_prayer = 100000;
    state.player.prayer_bonus = 0;
    state.player.prayer_drain_counter = initial_counter;
    state.player.weapon_uses_ammo = 1;
    state.player.ammo_count = 0;
    int expected_points = state.player.current_prayer;
    int expected_counter = initial_counter;
    int actions[FC_NUM_ACTION_HEADS];

    for (int tick = 0; tick < 100; tick++) {
        state.npcs[0].attack_timer = 0;
        neutral_actions(actions,
                        flick ? FC_PRAYER_FLICK_MELEE
                              : FC_PRAYER_NO_CHANGE,
                        FC_DRINK_NONE);
        fc_tick(&state, actions);
        if (!flick) {
            int resistance = 60;
            expected_counter += 12;
            while (expected_counter > resistance) {
                expected_counter -= resistance;
                expected_points -= 10;
            }
        }
        if (!state.player.hit_landed_this_tick ||
            !state.player.hit_blocked_this_tick ||
            state.player.current_prayer != expected_points ||
            state.player.prayer_drain_counter != expected_counter ||
            state.player.prayer != PRAYER_PROTECT_MELEE ||
            (flick && (!state.player.prayer_changed_this_tick ||
                       state.ep_prayer_switches != tick + 1))) {
            fprintf(stderr,
                    "FAIL PRAY-007: %s tick %d hit/blocked/points/counter/prayer/metric=%d/%d/%d/%d/%d/%d\n",
                    flick ? "flick" : "control", tick,
                    state.player.hit_landed_this_tick,
                    state.player.hit_blocked_this_tick,
                    state.player.current_prayer,
                    state.player.prayer_drain_counter,
                    state.player.prayer, state.ep_prayer_switches);
            return 1;
        }
    }
    return 0;
}

static int test_pray_007(void) {
    if (run_flick_trace(0, 1) || run_flick_trace(31, 1) ||
        run_flick_trace(0, 0)) {
        return 1;
    }
    printf("PASS PRAY-007: 100-tick flick traces and draining control\n");
    return 0;
}

static uint32_t seed_for_jad_style(int style) {
    for (uint32_t seed = 1; seed < 10000u; seed++) {
        FcState oracle;
        memset(&oracle, 0, sizeof(oracle));
        fc_rng_seed(&oracle, seed);
        int chosen = fc_rng_int(&oracle, 2) == 0
            ? ATTACK_MAGIC : ATTACK_RANGED;
        if (chosen == style) return seed;
    }
    return 0;
}

static int prayer_command_for_style(int style) {
    return style == ATTACK_MAGIC ? FC_PRAYER_MAGIC : FC_PRAYER_RANGE;
}

static int prayer_for_style(int style) {
    return style == ATTACK_MAGIC
        ? PRAYER_PROTECT_MAGIC : PRAYER_PROTECT_RANGE;
}

static int opposite_flick_for_style(int style) {
    return style == ATTACK_MAGIC
        ? FC_PRAYER_FLICK_RANGE : FC_PRAYER_FLICK_MAGIC;
}

static int finish_jad_hit(FcState* state, int first_command,
                          int expect_blocked) {
    int actions[FC_NUM_ACTION_HEADS];
    int hp_before = state->player.current_hp;
    for (int tick = 0; tick < 12 && state->player.num_pending_hits > 0; tick++) {
        int command = tick == 0 ? first_command : FC_PRAYER_NO_CHANGE;
        neutral_actions(actions, command, FC_DRINK_NONE);
        fc_tick(state, actions);
    }
    if (state->player.num_pending_hits != 0) {
        fprintf(stderr, "FAIL PRAY-008: Jad hit did not resolve in bounded trace\n");
        return 1;
    }
    int blocked = state->player.current_hp == hp_before;
    if (blocked != expect_blocked ||
        state->player.hit_blocked_this_tick != expect_blocked) {
        fprintf(stderr,
                "FAIL PRAY-008: impact blocked/hp-delta=%d/%d, expected %d/0-or-100\n",
                blocked, hp_before - state->player.current_hp,
                expect_blocked);
        return 1;
    }
    return 0;
}

static int run_jad_trace(int style, int late) {
    FcState state;
    make_npc_state(&state, NPC_TZTOK_JAD, 14);
    state.player.weapon_uses_ammo = 1;
    state.player.ammo_count = 0;
    uint32_t seed = seed_for_jad_style(style);
    if (seed == 0) {
        fprintf(stderr, "FAIL PRAY-008: no seed for Jad style %d\n", style);
        return 1;
    }
    fc_rng_seed(&state, seed);
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    int reveal_tick = state.tick;
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1) {
        fprintf(stderr, "FAIL PRAY-008: Jad did not queue exactly one hit\n");
        return 1;
    }
    FcPendingHit* hit = &state.player.pending_hits[0];
    int calculated = fc_npc_hit_delay(NPC_TZTOK_JAD, style,
                                      fc_distance_to_npc(state.player.x,
                                                         state.player.y,
                                                         &state.npcs[0]));
    int queued_delay = calculated < 3 ? 3 : calculated;
    if (hit->attack_style != style ||
        hit->prayer_lock_tick != reveal_tick + 2 ||
        hit->prayer_snapshot != -1 ||
        hit->ticks_remaining != queued_delay - 1) {
        fprintf(stderr,
                "FAIL PRAY-008: reveal style/lock/snapshot/delay=%d/%d/%d/%d, expected %d/%d/-1/%d\n",
                hit->attack_style, hit->prayer_lock_tick,
                hit->prayer_snapshot, hit->ticks_remaining,
                style, reveal_tick + 2, queued_delay - 1);
        return 1;
    }
    hit->damage = 100;
    float obs[FC_TOTAL_OBS];
    fc_write_obs(&state, obs);
    int npc_base = FC_OBS_NPC_START;
    if (obs[npc_base + FC_NPC_PENDING_PRAYER_WINDOW] != 1.0f ||
        obs[npc_base + FC_NPC_PENDING_PRAYER_DEADLINE] <= 0.0f) {
        fprintf(stderr, "FAIL PRAY-008: reveal observation lacks decision window\n");
        return 1;
    }

    neutral_actions(actions,
                    late ? FC_PRAYER_NO_CHANGE
                         : prayer_command_for_style(style),
                    FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].prayer_snapshot !=
            (late ? PRAYER_NONE : prayer_for_style(style))) {
        fprintf(stderr,
                "FAIL PRAY-008: T+1 boundary snapshot=%d, expected %d\n",
                state.player.num_pending_hits > 0
                    ? state.player.pending_hits[0].prayer_snapshot : -99,
                late ? PRAYER_NONE : prayer_for_style(style));
        return 1;
    }
    fc_write_obs(&state, obs);
    if (obs[npc_base + FC_NPC_PENDING_PRAYER_WINDOW] != 0.0f ||
        obs[npc_base + FC_NPC_PENDING_PRAYER_DEADLINE] != 0.0f) {
        fprintf(stderr, "FAIL PRAY-008: T+2 observation still says actionable\n");
        return 1;
    }

    int snapshot = state.player.pending_hits[0].prayer_snapshot;
    int first_command = late ? prayer_command_for_style(style)
                             : opposite_flick_for_style(style);
    if (finish_jad_hit(&state, first_command, !late)) return 1;
    if (snapshot != (late ? PRAYER_NONE : prayer_for_style(style))) {
        fprintf(stderr, "FAIL PRAY-008: stored lock expectation mutated\n");
        return 1;
    }
    return 0;
}

static int test_long_jad_delay(void) {
    FcState state;
    make_npc_state(&state, NPC_TZTOK_JAD, 24);
    state.player.weapon_uses_ammo = 1;
    state.player.ammo_count = 0;
    fc_rng_seed(&state, seed_for_jad_style(ATTACK_MAGIC));
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    int reveal_tick = state.tick;
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].attack_style != ATTACK_MAGIC ||
        state.player.pending_hits[0].prayer_lock_tick != reveal_tick + 2 ||
        state.player.pending_hits[0].ticks_remaining <= 2) {
        fprintf(stderr, "FAIL PRAY-008: long Magic reveal contract mismatch\n");
        return 1;
    }
    state.player.pending_hits[0].damage = 100;
    neutral_actions(actions, FC_PRAYER_MAGIC, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.pending_hits[0].prayer_snapshot !=
        PRAYER_PROTECT_MAGIC) {
        fprintf(stderr, "FAIL PRAY-008: long hit did not lock at T+2 boundary\n");
        return 1;
    }
    return finish_jad_hit(&state, FC_PRAYER_OFF, 1);
}

static int test_ordinary_aging_unchanged(void) {
    FcState state;
    make_open_state(&state);
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 0, 2, ATTACK_MAGIC, 0, 0);
    state.player.pending_hits[0].prayer_snapshot = PRAYER_NONE;
    int actions[FC_NUM_ACTION_HEADS];
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1 ||
        state.player.pending_hits[0].ticks_remaining != 1) {
        fprintf(stderr,
                "FAIL PRAY-008: ordinary delay-2 aging changed globally\n");
        return 1;
    }
    return 0;
}

static int test_pray_008(void) {
    static const int styles[] = {ATTACK_MAGIC, ATTACK_RANGED};
    for (size_t i = 0; i < sizeof(styles) / sizeof(styles[0]); i++) {
        if (run_jad_trace(styles[i], 0) || run_jad_trace(styles[i], 1)) {
            return 1;
        }
    }
    if (test_long_jad_delay() || test_ordinary_aging_unchanged()) return 1;
    printf("PASS PRAY-008: Jad reveal, lock, observation, and impact traces\n");
    return 0;
}

static int test_obs_001(void) {
    const float eps = 0.0001f;
    FcState state;
    float obs[FC_TOTAL_OBS];
    int actions[FC_NUM_ACTION_HEADS];

    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = state.player.max_prayer;
    state.player.prayer_drain_counter = 31;
    int resistance = 60 + 2 * state.player.prayer_bonus;
    neutral_actions(actions, FC_PRAYER_FLICK_RANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    fc_write_obs(&state, obs);
    if (obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_MEL] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_RNG] != 1.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_MAG] != 0.0f ||
        fabsf(obs[FC_OBS_META_START + FC_OBS_META_PRAY_DRAIN] -
              (float)31 / (float)resistance) > eps ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAYER_LOST] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_OVERHEAD_PRAYER_LOST] != 0.0f) {
        fprintf(stderr,
                "FAIL OBS-001: flick final-overhead/counter/loss observation mismatch\n");
        return 1;
    }

    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 10;
    state.player.prayer_drain_counter =
        60 + 2 * state.player.prayer_bonus;
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    fc_write_obs(&state, obs);
    float expected_loss = (float)10 / (float)state.player.max_prayer;
    if (obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAYER] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_MEL] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_RNG] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_MAG] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_PRAY_DRAIN] != 0.0f ||
        fabsf(obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAYER_LOST] -
              expected_loss) > eps ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_OVERHEAD_PRAYER_LOST] != 1.0f) {
        fprintf(stderr,
                "FAIL OBS-001: passive-depletion observation mismatch\n");
        return 1;
    }

    make_npc_state(&state, NPC_TZ_KIH, 11);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 10;
    state.player.prayer_drain_counter = 37;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 0, 1, ATTACK_MELEE, 0,
                         fc_npc_get_stats(NPC_TZ_KIH)->prayer_drain);
    state.player.pending_hits[0].prayer_snapshot = PRAYER_PROTECT_MAGIC;
    fc_resolve_player_pending_hits(&state);
    fc_write_obs(&state, obs);
    if (obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAYER] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_PRAY_DRAIN] != 0.0f ||
        fabsf(obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAYER_LOST] -
              expected_loss) > eps ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_OVERHEAD_PRAYER_LOST] != 0.0f ||
        obs[FC_OBS_NPC_START + FC_NPC_PRAYER_DRAIN_DEALT] <= 0.0f) {
        fprintf(stderr,
                "FAIL OBS-001: Tz-Kih-depletion observation mismatch\n");
        return 1;
    }

    printf("PASS OBS-001: final overhead, drain fraction, and Prayer-loss observations\n");
    return 0;
}

static int test_obs_002(void) {
    const float eps = 0.0001f;
    FcState state;
    float obs[FC_TOTAL_OBS];
    int actions[FC_NUM_ACTION_HEADS];

    make_npc_state(&state, NPC_TZTOK_JAD, 14);
    state.player.weapon_uses_ammo = 1;
    state.player.ammo_count = 0;
    fc_rng_seed(&state, seed_for_jad_style(ATTACK_MAGIC));
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.num_pending_hits != 1 ||
        state.tick + 1 != state.player.pending_hits[0].prayer_lock_tick) {
        fprintf(stderr,
                "FAIL OBS-002: fixture did not reach Jad's final valid decision\n");
        return 1;
    }

    fc_write_obs(&state, obs);
    float player_deadline =
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_MAG];
    float npc_window =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_WINDOW];
    float npc_deadline =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_DEADLINE];
    if (fabsf(player_deadline - 1.0f) > eps || npc_window != 1.0f ||
        fabsf(npc_deadline - 1.0f) > eps) {
        fprintf(stderr,
                "FAIL OBS-002: final valid decision urgency player/window/npc=%.3f/%.3f/%.3f, expected 1/1/1\n",
                player_deadline, npc_window, npc_deadline);
        return 1;
    }

    printf("PASS OBS-002: Jad's final valid decision has maximum urgency\n");
    return 0;
}

static int test_obs_003(void) {
    FcState state;
    float obs[FC_TOTAL_OBS];

    make_npc_state(&state, NPC_TZTOK_JAD, 14);
    state.tick = 77;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 100, 3, ATTACK_RANGED, 0, 0);
    state.player.pending_hits[0].prayer_snapshot = -1;
    state.player.pending_hits[0].prayer_lock_tick = state.tick;
    fc_write_obs(&state, obs);

    float player_deadline =
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_RNG];
    float npc_window =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_WINDOW];
    float npc_deadline =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_DEADLINE];
    float pending_style =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_STYLE];
    float pending_ticks =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_TICKS];
    if (player_deadline != 0.0f || npc_window != 0.0f ||
        npc_deadline != 0.0f || pending_style <= 0.0f ||
        pending_ticks <= 0.0f) {
        fprintf(stderr,
                "FAIL OBS-003: lock-tick unset snapshot reports player/window/npc/style/ticks=%.3f/%.3f/%.3f/%.3f/%.3f\n",
                player_deadline, npc_window, npc_deadline,
                pending_style, pending_ticks);
        return 1;
    }

    printf("PASS OBS-003: lock-tick hits are non-actionable even if snapshot is unset\n");
    return 0;
}

static int test_obs_004(void) {
    FcState state;
    float obs[FC_TOTAL_OBS];

    make_npc_state(&state, NPC_TZTOK_JAD, 14);
    state.tick = 100;
    fc_queue_pending_hit(state.player.pending_hits,
                         &state.player.num_pending_hits,
                         FC_MAX_PENDING_HITS, 100, 2, ATTACK_MAGIC, 0, 0);
    state.player.pending_hits[0].prayer_snapshot = -1;
    state.player.pending_hits[0].prayer_lock_tick = state.tick + 1;
    fc_write_obs(&state, obs);

    float player_deadline =
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_MAG];
    float npc_window =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_WINDOW];
    float npc_deadline =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_DEADLINE];
    float pending_style =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_STYLE];
    float pending_ticks =
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_TICKS];
    if (obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MAG_2T] <= 0.0f ||
        player_deadline <= 0.0f || npc_window != 1.0f ||
        npc_deadline <= 0.0f) {
        fprintf(stderr,
                "FAIL OBS-004: pre-ablation fixture lacks aggregate/deadline signals\n");
        return 1;
    }

    fc_apply_obs_ablation(obs, 0, 1, 0);
    if (obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MEL_1T] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_RNG_1T] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MAG_1T] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MEL_2T] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_RNG_2T] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_IN_MAG_2T] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_IN_MEL_3T] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_IN_RNG_3T] != 0.0f ||
        obs[FC_OBS_META_START + FC_OBS_META_IN_MAG_3T] != 0.0f ||
        obs[FC_OBS_PLAYER_START + FC_OBS_PLAYER_PRAY_DDL_MAG] !=
            player_deadline ||
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_WINDOW] != npc_window ||
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_PRAYER_DEADLINE] !=
            npc_deadline ||
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_STYLE] != pending_style ||
        obs[FC_OBS_NPC_START + FC_NPC_PENDING_TICKS] != pending_ticks) {
        fprintf(stderr,
                "FAIL OBS-004: incoming aggregate ablation changed deadline/per-NPC signals\n");
        return 1;
    }

    printf("PASS OBS-004: aggregate ablation preserves deadline and per-NPC signals\n");
    return 0;
}

static FcRewardBreakdown prayer_reward_breakdown(
        const FcState* state, float jad_weight, float danger_weight,
        float loss_weight, float invalid_weight, float unnecessary_weight) {
    FcRewardParams params = {0};
    FcRewardRuntime runtime;

    params.w_correct_jad_prayer = jad_weight;
    params.w_correct_danger_prayer = danger_weight;
    params.w_prayer_lost = loss_weight;
    params.w_invalid_action = invalid_weight;
    params.shape_unnecessary_prayer_penalty = unnecessary_weight;
    fc_reward_runtime_reset(&runtime);
    return fc_reward_compute_breakdown(state, &params, &runtime);
}

static int queue_locked_hit(FcState* state, int damage, int style,
                            int source_idx, int prayer_drain,
                            int prayer_snapshot) {
    if (!fc_queue_pending_hit(state->player.pending_hits,
                              &state->player.num_pending_hits,
                              FC_MAX_PENDING_HITS, damage, 1, style,
                              source_idx, prayer_drain)) {
        return 0;
    }
    state->player.pending_hits[state->player.num_pending_hits - 1]
        .prayer_snapshot = prayer_snapshot;
    return 1;
}

static int test_rwd_001(void) {
    const float eps = 0.0001f;
    FcState state;
    FcRewardBreakdown reward;

    make_npc_state(&state, NPC_KET_ZEK, 20);
    state.player.prayer = PRAYER_PROTECT_RANGE;
    int hp_before = state.player.current_hp;
    if (!queue_locked_hit(&state, 100, ATTACK_MAGIC, 0, 0,
                          PRAYER_PROTECT_MAGIC)) {
        fprintf(stderr, "FAIL RWD-001: could not queue ordinary correct hit\n");
        return 1;
    }
    fc_resolve_player_pending_hits(&state);
    reward = prayer_reward_breakdown(&state, 0.0f, 0.005f,
                                     0.0f, 0.0f, 0.0f);
    if (state.player.current_hp != hp_before ||
        state.player.hit_locked_prayer_this_tick != PRAYER_PROTECT_MAGIC ||
        !state.player.hit_blocked_this_tick ||
        !state.correct_danger_prayer || state.wrong_danger_prayer ||
        state.ep_correct_blocks != 1 || state.ep_wrong_prayer_hits != 0 ||
        state.ep_no_prayer_hits != 0 ||
        reward.raw[FC_RWD_CORRECT_DANGER_PRAY] != 1.0f ||
        reward.raw[FC_RWD_WRONG_DANGER_PRAY] != 0.0f ||
        fabsf(reward.correct_danger_prayer - 0.005f) > eps ||
        fabsf(reward.total - 0.005f) > eps) {
        fprintf(stderr,
                "FAIL RWD-001: ordinary correct reward/metrics reread live prayer\n");
        return 1;
    }

    make_npc_state(&state, NPC_KET_ZEK, 20);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    hp_before = state.player.current_hp;
    if (!queue_locked_hit(&state, 100, ATTACK_MAGIC, 0, 0,
                          PRAYER_PROTECT_RANGE)) {
        fprintf(stderr, "FAIL RWD-001: could not queue ordinary wrong hit\n");
        return 1;
    }
    fc_resolve_player_pending_hits(&state);
    reward = prayer_reward_breakdown(&state, 0.0f, 0.005f,
                                     0.0f, 0.0f, 0.0f);
    if (state.player.current_hp != hp_before - 100 ||
        state.player.hit_locked_prayer_this_tick != PRAYER_PROTECT_RANGE ||
        state.player.hit_blocked_this_tick || state.correct_danger_prayer ||
        !state.wrong_danger_prayer || state.ep_correct_blocks != 0 ||
        state.ep_wrong_prayer_hits != 1 || state.ep_no_prayer_hits != 0 ||
        reward.raw[FC_RWD_CORRECT_DANGER_PRAY] != 0.0f ||
        reward.raw[FC_RWD_WRONG_DANGER_PRAY] != 1.0f ||
        reward.correct_danger_prayer != 0.0f || reward.total != 0.0f) {
        fprintf(stderr,
                "FAIL RWD-001: ordinary wrong reward/metrics reread live prayer\n");
        return 1;
    }

    make_npc_state(&state, NPC_TZTOK_JAD, 14);
    state.player.prayer = PRAYER_PROTECT_RANGE;
    hp_before = state.player.current_hp;
    if (!queue_locked_hit(&state, 500, ATTACK_MAGIC, 0, 0,
                          PRAYER_PROTECT_MAGIC)) {
        fprintf(stderr, "FAIL RWD-001: could not queue Jad correct hit\n");
        return 1;
    }
    fc_resolve_player_pending_hits(&state);
    reward = prayer_reward_breakdown(&state, 0.25f, 0.005f,
                                     0.0f, 0.0f, 0.0f);
    if (state.player.current_hp != hp_before || !state.correct_jad_prayer ||
        !state.correct_danger_prayer || state.wrong_jad_prayer ||
        state.ep_correct_blocks != 1 ||
        reward.raw[FC_RWD_CORRECT_JAD_PRAY] != 1.0f ||
        reward.raw[FC_RWD_CORRECT_DANGER_PRAY] != 1.0f ||
        fabsf(reward.correct_jad_prayer - 0.25f) > eps ||
        fabsf(reward.correct_danger_prayer - 0.005f) > eps ||
        fabsf(reward.total - 0.255f) > eps) {
        fprintf(stderr,
                "FAIL RWD-001: Jad correct reward/metrics ignored locked prayer\n");
        return 1;
    }

    make_npc_state(&state, NPC_TZTOK_JAD, 14);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    hp_before = state.player.current_hp;
    if (!queue_locked_hit(&state, 500, ATTACK_MAGIC, 0, 0, PRAYER_NONE)) {
        fprintf(stderr, "FAIL RWD-001: could not queue Jad late-control hit\n");
        return 1;
    }
    fc_resolve_player_pending_hits(&state);
    reward = prayer_reward_breakdown(&state, 0.25f, 0.005f,
                                     0.0f, 0.0f, 0.0f);
    if (state.player.current_hp != hp_before - 500 ||
        state.player.hit_locked_prayer_this_tick != PRAYER_NONE ||
        state.player.hit_blocked_this_tick || state.correct_jad_prayer ||
        !state.wrong_jad_prayer || state.ep_correct_blocks != 0 ||
        state.ep_wrong_prayer_hits != 0 || state.ep_no_prayer_hits != 1 ||
        reward.raw[FC_RWD_CORRECT_JAD_PRAY] != 0.0f ||
        reward.raw[FC_RWD_CORRECT_DANGER_PRAY] != 0.0f ||
        reward.correct_jad_prayer != 0.0f ||
        reward.correct_danger_prayer != 0.0f || reward.total != 0.0f) {
        fprintf(stderr,
                "FAIL RWD-001: post-lock Jad prayer changed reward/metrics\n");
        return 1;
    }

    printf("PASS RWD-001: reward flags and hit metrics use immutable prayer snapshots\n");
    return 0;
}

static int test_rwd_002(void) {
    const float eps = 0.0001f;
    FcState state;
    FcRewardBreakdown reward;
    float features[FC_REWARD_FEATURES];
    int actions[FC_NUM_ACTION_HEADS];

    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 100;
    state.player.prayer_drain_counter = 59;
    neutral_actions(actions, FC_PRAYER_FLICK_MAGIC, FC_DRINK_NONE);
    fc_tick(&state, actions);
    fc_write_reward_features(&state, features);
    reward = prayer_reward_breakdown(&state, 0.0f, 0.0f,
                                     -0.02f, 0.0f, 0.0f);
    if (state.prayer_lost_this_tick != 0 ||
        state.overhead_prayer_lost_this_tick != 0 ||
        state.tz_kih_prayer_drain_this_tick != 0 ||
        features[FC_RWD_PRAYER_LOST] != 0.0f ||
        reward.prayer_lost != 0.0f || reward.total != 0.0f) {
        fprintf(stderr, "FAIL RWD-002: drain-free flick produced Prayer penalty\n");
        return 1;
    }

    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 100;
    state.player.prayer_bonus = 0;
    state.player.prayer_drain_counter = 60;
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    fc_write_reward_features(&state, features);
    reward = prayer_reward_breakdown(&state, 0.0f, 0.0f,
                                     -0.02f, 0.0f, 0.0f);
    if (state.prayer_lost_this_tick != 10 ||
        state.overhead_prayer_lost_this_tick != 10 ||
        state.tz_kih_prayer_drain_this_tick != 0 ||
        fabsf(features[FC_RWD_PRAYER_LOST] - 1.0f) > eps ||
        fabsf(reward.prayer_lost + 0.02f) > eps ||
        fabsf(reward.total + 0.02f) > eps) {
        fprintf(stderr,
                "FAIL RWD-002: passive Prayer loss reward was not actual loss\n");
        return 1;
    }

    make_npc_state(&state, NPC_TZ_KIH, 11);
    state.npcs[0].attack_timer = 100;
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 20;
    state.player.prayer_drain_counter = 37;
    if (!queue_locked_hit(&state, 0, ATTACK_MELEE, 0, 10,
                          PRAYER_PROTECT_MAGIC)) {
        fprintf(stderr, "FAIL RWD-002: could not queue Tz-Kih flick hit\n");
        return 1;
    }
    neutral_actions(actions, FC_PRAYER_FLICK_MAGIC, FC_DRINK_NONE);
    fc_tick(&state, actions);
    fc_write_reward_features(&state, features);
    reward = prayer_reward_breakdown(&state, 0.0f, 0.0f,
                                     -0.02f, 0.0f, 0.0f);
    if (state.prayer_lost_this_tick != 10 ||
        state.overhead_prayer_lost_this_tick != 0 ||
        state.tz_kih_prayer_drain_this_tick != 10 ||
        fabsf(features[FC_RWD_PRAYER_LOST] - 1.0f) > eps ||
        fabsf(reward.prayer_lost + 0.02f) > eps ||
        fabsf(reward.total + 0.02f) > eps) {
        fprintf(stderr,
                "FAIL RWD-002: Tz-Kih drain during flick lost its penalty\n");
        return 1;
    }

    make_npc_state(&state, NPC_TZ_KIH, 11);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 5;
    if (!queue_locked_hit(&state, 0, ATTACK_MELEE, 0, 10,
                          PRAYER_PROTECT_MAGIC)) {
        fprintf(stderr, "FAIL RWD-002: could not queue capped Tz-Kih hit\n");
        return 1;
    }
    fc_resolve_player_pending_hits(&state);
    fc_write_reward_features(&state, features);
    reward = prayer_reward_breakdown(&state, 0.0f, 0.0f,
                                     -0.02f, 0.0f, 0.0f);
    if (state.player.current_prayer != 0 ||
        state.prayer_lost_this_tick != 5 ||
        state.tz_kih_prayer_drain_this_tick != 5 ||
        fabsf(features[FC_RWD_PRAYER_LOST] - 0.5f) > eps ||
        fabsf(reward.prayer_lost + 0.01f) > eps ||
        fabsf(reward.total + 0.01f) > eps) {
        fprintf(stderr,
                "FAIL RWD-002: capped Tz-Kih loss reward used requested loss\n");
        return 1;
    }

    printf("PASS RWD-002: Prayer-loss rewards use actual passive and Tz-Kih loss\n");
    return 0;
}

static int test_rwd_003(void) {
    const float eps = 0.0001f;
    FcRewardParams defaults = fc_reward_default_params();
    FcState state;
    FcRewardBreakdown reward;
    float features[FC_REWARD_FEATURES];
    int actions[FC_NUM_ACTION_HEADS];

    if (FC_REWARD_FEATURES != 20 ||
        fabsf(defaults.w_correct_jad_prayer - 0.0f) > eps ||
        fabsf(defaults.w_correct_danger_prayer - 0.005f) > eps ||
        fabsf(defaults.w_prayer_lost + 0.02f) > eps ||
        fabsf(defaults.w_invalid_action + 0.1f) > eps ||
        fabsf(defaults.shape_unnecessary_prayer_penalty - 0.0f) > eps) {
        fprintf(stderr, "FAIL RWD-003: reward layout or Prayer defaults drifted\n");
        return 1;
    }

    for (int command = FC_PRAYER_FLICK_MAGIC;
         command <= FC_PRAYER_FLICK_MELEE; command++) {
        make_open_state(&state);
        state.player.current_prayer = 0;
        neutral_actions(actions, command, FC_DRINK_NONE);
        fc_tick(&state, actions);
        fc_write_reward_features(&state, features);
        reward = prayer_reward_breakdown(&state, 0.0f, 0.0f,
                                         0.0f, -0.1f, 0.0f);
        if (state.invalid_action_this_tick ||
            state.invalid_action_class_this_tick[FC_INVALID_ACTION_PRAYER] ||
            state.ep_invalid_action_classes[FC_INVALID_ACTION_PRAYER] != 0 ||
            state.ep_action_prayer_cmd_ticks != 1 ||
            features[FC_RWD_INVALID_ACTION] != 0.0f ||
            reward.invalid_action != 0.0f || reward.total != 0.0f) {
            fprintf(stderr,
                    "FAIL RWD-003: flick command %d was penalized as invalid\n",
                    command);
            return 1;
        }
    }

    static const int invalid_commands[] = {-1, FC_PRAYER_DIM};
    for (size_t i = 0;
         i < sizeof(invalid_commands) / sizeof(invalid_commands[0]); i++) {
        make_open_state(&state);
        neutral_actions(actions, invalid_commands[i], FC_DRINK_NONE);
        fc_tick(&state, actions);
        fc_write_reward_features(&state, features);
        reward = prayer_reward_breakdown(&state, 0.0f, 0.0f,
                                         0.0f, -0.1f, 0.0f);
        if (!state.invalid_action_this_tick ||
            !state.invalid_action_class_this_tick[FC_INVALID_ACTION_PRAYER] ||
            state.ep_invalid_action_classes[FC_INVALID_ACTION_PRAYER] != 1 ||
            features[FC_RWD_INVALID_ACTION] != 1.0f ||
            fabsf(reward.invalid_action + 0.1f) > eps ||
            fabsf(reward.total + 0.1f) > eps) {
            fprintf(stderr,
                    "FAIL RWD-003: out-of-range command %d lacked invalid penalty\n",
                    invalid_commands[i]);
            return 1;
        }
    }

    make_open_state(&state);
    neutral_actions(actions, FC_PRAYER_FLICK_MAGIC, FC_DRINK_NONE);
    fc_tick(&state, actions);
    reward = prayer_reward_breakdown(&state, 0.0f, 0.0f,
                                     0.0f, 0.0f, -0.25f);
    if (state.player.prayer != PRAYER_PROTECT_MAGIC ||
        fabsf(reward.unnecessary_prayer + 0.25f) > eps ||
        fabsf(reward.total + 0.25f) > eps) {
        fprintf(stderr,
                "FAIL RWD-003: unnecessary-prayer shaping ignored final overhead\n");
        return 1;
    }

    printf("PASS RWD-003: reward layout, defaults, flick validity, and final-state shaping\n");
    return 0;
}

static int test_metric_001(void) {
    FcState state;
    int actions[FC_NUM_ACTION_HEADS];

    make_open_state(&state);
    neutral_actions(actions, FC_PRAYER_MAGIC, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.ep_ticks_pray_melee != 0 || state.ep_ticks_pray_range != 0 ||
        state.ep_ticks_pray_magic != 0) {
        fprintf(stderr,
                "FAIL METRIC-001: activation tick counted final prayer uptime=%d/%d/%d\n",
                state.ep_ticks_pray_melee, state.ep_ticks_pray_range,
                state.ep_ticks_pray_magic);
        return 1;
    }

    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    neutral_actions(actions, FC_PRAYER_OFF, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.ep_ticks_pray_magic != 1 || state.ep_ticks_pray_range != 0 ||
        state.ep_ticks_pray_melee != 0) {
        fprintf(stderr,
                "FAIL METRIC-001: deactivation tick omitted start prayer uptime\n");
        return 1;
    }

    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    neutral_actions(actions, FC_PRAYER_RANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.ep_ticks_pray_magic != 1 || state.ep_ticks_pray_range != 0) {
        fprintf(stderr,
                "FAIL METRIC-001: direct switch counted final instead of start prayer\n");
        return 1;
    }

    make_open_state(&state);
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    neutral_actions(actions, FC_PRAYER_FLICK_RANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.ep_ticks_pray_magic != 1 || state.ep_ticks_pray_range != 0 ||
        state.ep_prayer_switches != 1) {
        fprintf(stderr,
                "FAIL METRIC-001: flick uptime/transition metrics used final prayer\n");
        return 1;
    }

    make_npc_state(&state, NPC_TZ_KIH, 11);
    state.npcs[0].attack_timer = 100;
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = 10;
    state.player.prayer_bonus = 0;
    if (!queue_locked_hit(&state, 0, ATTACK_MELEE, 0, 10,
                          PRAYER_PROTECT_MAGIC)) {
        fprintf(stderr, "FAIL METRIC-001: could not queue depletion hit\n");
        return 1;
    }
    neutral_actions(actions, FC_PRAYER_NO_CHANGE, FC_DRINK_NONE);
    fc_tick(&state, actions);
    if (state.player.prayer != PRAYER_NONE ||
        state.ep_ticks_pray_magic != 1 || state.ep_ticks_pray_range != 0 ||
        state.ep_ticks_pray_melee != 0) {
        fprintf(stderr,
                "FAIL METRIC-001: depletion tick omitted effective start prayer\n");
        return 1;
    }

    printf("PASS METRIC-001: Prayer uptime uses the effective tick-start overhead\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr,
                "usage: %s <pray_001|pray_002|pray_003|pray_004|pray_005|pray_006|pray_007|pray_008|obs_001|obs_002|obs_003|obs_004|rwd_001|rwd_002|rwd_003|metric_001>\n",
                argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "pray_001") == 0) return test_pray_001();
    if (strcmp(argv[1], "pray_002") == 0) return test_pray_002();
    if (strcmp(argv[1], "pray_003") == 0) return test_pray_003();
    if (strcmp(argv[1], "pray_004") == 0) return test_pray_004();
    if (strcmp(argv[1], "pray_005") == 0) return test_pray_005();
    if (strcmp(argv[1], "pray_006") == 0) return test_pray_006();
    if (strcmp(argv[1], "pray_007") == 0) return test_pray_007();
    if (strcmp(argv[1], "pray_008") == 0) return test_pray_008();
    if (strcmp(argv[1], "obs_001") == 0) return test_obs_001();
    if (strcmp(argv[1], "obs_002") == 0) return test_obs_002();
    if (strcmp(argv[1], "obs_003") == 0) return test_obs_003();
    if (strcmp(argv[1], "obs_004") == 0) return test_obs_004();
    if (strcmp(argv[1], "rwd_001") == 0) return test_rwd_001();
    if (strcmp(argv[1], "rwd_002") == 0) return test_rwd_002();
    if (strcmp(argv[1], "rwd_003") == 0) return test_rwd_003();
    if (strcmp(argv[1], "metric_001") == 0) return test_metric_001();

    fprintf(stderr, "unknown parity prayer test: %s\n", argv[1]);
    return 2;
}
