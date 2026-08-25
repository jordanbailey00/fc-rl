#include "fc_api.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int summary_values_are_exact(void) {
    const float epsilon = 0.000001f;
    FcState state = {0};
    state.current_wave = 63;
    state.total_npcs_killed = 275;
    state.terminal = TERMINAL_PLAYER_DEATH;
    state.player.total_damage_taken = 890;
    state.ep_ticks_pray_melee = 2;
    state.ep_ticks_pray_range = 3;
    state.ep_ticks_pray_magic = 5;
    state.ep_correct_blocks = 11;
    state.ep_wrong_prayer_hits = 12;
    state.ep_no_prayer_hits = 13;
    state.ep_prayer_switches = 14;
    state.ep_damage_blocked = 15;
    state.ep_attack_ready_ticks = 8;
    state.ep_attack_attempt_ticks = 6;
    state.ep_invalid_action_classes[FC_INVALID_ACTION_MOVE] = 16;
    state.ep_invalid_action_classes[FC_INVALID_ACTION_ATTACK] = 17;
    state.ep_invalid_action_classes[FC_INVALID_ACTION_PRAYER] = 18;
    state.ep_tokxil_melee_ticks = 19;
    state.ep_ketzek_melee_ticks = 20;
    state.ep_max_wave_ticks = 21;
    state.ep_max_wave_ticks_wave = 22;
    state.ep_reached_wave_63 = 1;
    state.ep_jad_killed = 1;
    state.ep_target_held_ticks = 23;
    state.ep_no_target_ticks = 24;
    state.ep_target_in_range_los_ticks = 25;
    state.ep_target_out_of_range_or_los_ticks = 26;
    state.ep_attack_cooldown_wait_ticks = 27;
    state.ep_ready_but_no_attack_ticks = 28;
    state.ep_action_move_idle_ticks = 29;
    state.ep_action_move_walk_ticks = 30;
    state.ep_action_move_run_ticks = 31;
    state.ep_action_attack_none_ticks = 32;
    state.ep_action_attack_target_ticks = 33;
    state.ep_action_prayer_noop_ticks = 34;
    state.ep_action_prayer_cmd_ticks = 35;
    for (int i = 0; i < NPC_TYPE_COUNT; i++) {
        state.ep_damage_to_npc_type[i] = 100 + i;
        state.ep_resolved_hits_to_npc_type[i] = 200 + i;
        state.ep_damaging_hits_to_npc_type[i] = 300 + i;
        state.ep_attack_cycles_to_npc_type[i] = 400 + i;
        state.ep_target_ticks_by_npc_type[i] = 500 + i;
    }
    FcState before = state;

    FcEpisodeSummary summary;
    fc_episode_summary_build(&state, 10, &summary);

    if (memcmp(&state, &before, sizeof(state)) != 0 ||
        summary.episode_length != 10 || summary.wave_reached != 63 ||
        summary.npcs_slayed != 275 ||
        fabsf(summary.prayer_uptime_melee - 0.2f) > epsilon ||
        fabsf(summary.prayer_uptime_range - 0.3f) > epsilon ||
        fabsf(summary.prayer_uptime_magic - 0.5f) > epsilon ||
        summary.correct_prayer != 11 || summary.wrong_prayer_hits != 12 ||
        summary.no_prayer_hits != 13 || summary.prayer_switches != 14 ||
        summary.damage_blocked != 15 || summary.damage_taken != 890 ||
        fabsf(summary.attack_when_ready_rate - 0.75f) > epsilon ||
        summary.invalid_move != 16 || summary.invalid_attack != 17 ||
        summary.invalid_prayer != 18 || summary.tokxil_melee_ticks != 19 ||
        summary.ketzek_melee_ticks != 20 || summary.max_wave_ticks != 21 ||
        summary.max_wave_ticks_wave != 22 || summary.reached_wave_63 != 1 ||
        summary.jad_killed != 1 || summary.player_died != 1 ||
        summary.target_held_ticks != 23 || summary.no_target_ticks != 24 ||
        summary.target_in_range_los_ticks != 25 ||
        summary.target_out_of_range_or_los_ticks != 26 ||
        summary.attack_cooldown_wait_ticks != 27 ||
        summary.ready_but_no_attack_ticks != 28 ||
        summary.action_move_idle_ticks != 29 ||
        summary.action_move_walk_ticks != 30 ||
        summary.action_move_run_ticks != 31 ||
        summary.action_attack_none_ticks != 32 ||
        summary.action_attack_target_ticks != 33 ||
        summary.action_prayer_noop_ticks != 34 ||
        summary.action_prayer_cmd_ticks != 35) {
        fprintf(stderr, "episode summary scalar derivation drifted\n");
        return 0;
    }

    for (int i = 0; i < NPC_TYPE_COUNT; i++) {
        if (summary.damage_to_npc_type[i] != 100 + i ||
            summary.resolved_hits_to_npc_type[i] != 200 + i ||
            summary.damaging_hits_to_npc_type[i] != 300 + i ||
            summary.attack_cycles_to_npc_type[i] != 400 + i ||
            summary.target_ticks_by_npc_type[i] != 500 + i) {
            fprintf(stderr, "episode summary NPC metric %d drifted\n", i);
            return 0;
        }
    }
    return 1;
}

static int zero_denominators_are_safe(void) {
    FcState state = {0};
    FcEpisodeSummary summary;
    fc_episode_summary_build(&state, 0, &summary);
    return summary.prayer_uptime_melee == 0.0f &&
           summary.prayer_uptime_range == 0.0f &&
           summary.prayer_uptime_magic == 0.0f &&
           summary.attack_when_ready_rate == 0.0f;
}

static int metric_names_are_stable(void) {
    static const char* expected[NPC_TYPE_COUNT] = {
        "none", "tz_kih", "tz_kek", "tz_kek_sm", "tok_xil",
        "yt_mejkot", "ket_zek", "tztok_jad", "yt_hurkot",
    };
    for (int i = 0; i < NPC_TYPE_COUNT; i++) {
        if (strcmp(fc_episode_npc_metric_name(i), expected[i]) != 0)
            return 0;
    }
    return strcmp(fc_episode_npc_metric_name(-1), "unknown") == 0 &&
           strcmp(fc_episode_npc_metric_name(NPC_TYPE_COUNT), "unknown") == 0;
}

int main(void) {
    if (!summary_values_are_exact() || !zero_denominators_are_safe() ||
        !metric_names_are_stable()) {
        return 1;
    }
    printf("PASS: shared episode summary is exact, read-only, and stable\n");
    return 0;
}
