#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_player_init.h"
#include "fc_reward.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FC_REPLAY_BACKEND_NAME
#define FC_REPLAY_BACKEND_NAME "unknown"
#endif

#define TRACE_COUNT 8
#define TRACE_TICKS 160

enum { REPLAY_FLICK_PERIOD_TICKS = 5 };

static uint32_t action_next(uint32_t* state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static uint32_t float_bits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static void apply_loadout(FcState* state, int loadout_id) {
    const FcLoadout* loadout = &FC_LOADOUTS[loadout_id];
    FcPlayer* player = &state->player;

    player->max_hp = loadout->max_hp;
    player->current_hp = loadout->max_hp;
    player->max_prayer = loadout->max_prayer;
    player->current_prayer = loadout->max_prayer;
    player->attack_level = loadout->attack_lvl;
    player->strength_level = loadout->strength_lvl;
    player->defence_level = loadout->defence_lvl;
    player->ranged_level = loadout->ranged_lvl;
    player->prayer_level = loadout->prayer_lvl;
    player->magic_level = loadout->magic_lvl;
    player->weapon_kind = loadout->weapon_kind;
    player->weapon_uses_ammo = loadout->weapon_uses_ammo;
    player->crystal_piece_mask = loadout->crystal_piece_mask;
    player->weapon_speed = loadout->weapon_speed;
    player->weapon_range = loadout->weapon_range;
    player->ranged_attack_bonus = loadout->ranged_atk;
    player->ranged_strength_bonus = loadout->ranged_str;
    player->defence_stab = loadout->def_stab;
    player->defence_slash = loadout->def_slash;
    player->defence_crush = loadout->def_crush;
    player->defence_magic = loadout->def_magic;
    player->defence_ranged = loadout->def_ranged;
    player->prayer_bonus = loadout->prayer_bonus;
    player->ammo_count = loadout->ammo;
    state->active_loadout = loadout_id;
}

static void clear_npcs(FcState* state) {
    memset(state->npcs, 0, sizeof(state->npcs));
    state->npcs_remaining = 0;
    state->next_spawn_index = 0;
}

static void spawn_npc(FcState* state, int type, int x, int y) {
    int slot = state->next_spawn_index;
    if (slot >= FC_MAX_NPCS) abort();
    fc_npc_spawn(&state->npcs[slot], type, x, y, slot);
    state->npcs[slot].attack_timer = slot % 3;
    state->next_spawn_index++;
    state->npcs_remaining++;
}

static int trace_loadout(int trace_id) {
    return trace_id == 6 ? FC_LOADOUT_BOWFA_CRYSTAL : FC_LOADOUT_SOTA_TBOW;
}

static void setup_trace(FcState* state, int trace_id, uint32_t seed,
                        FcRewardRuntime* runtime) {
    fc_reset(state, seed);
    clear_npcs(state);
    apply_loadout(state, trace_loadout(trace_id));
    state->player.x = 32;
    state->player.y = 32;
    state->player.attack_target_idx = -1;
    state->player.approach_target = 0;
    state->player.route_len = 0;
    state->player.route_idx = 0;
    state->player.sharks_remaining = 1;
    state->player.prayer_doses_remaining = 1;

    switch (trace_id) {
        case 0:
            state->current_wave = 31;
            spawn_npc(state, NPC_KET_ZEK, 42, 32);
            break;
        case 1:
            state->current_wave = 15;
            spawn_npc(state, NPC_TOK_XIL, 39, 32);
            break;
        case 2:
            state->current_wave = 3;
            spawn_npc(state, NPC_TZ_KIH, 33, 32);
            break;
        case 3:
            state->current_wave = 63;
            spawn_npc(state, NPC_TOK_XIL, 33, 32);
            spawn_npc(state, NPC_KET_ZEK, 43, 32);
            spawn_npc(state, NPC_TZTOK_JAD, 50, 32);
            break;
        case 4:
            state->current_wave = 7;
            state->player.current_prayer = 10;
            state->player.prayer_drain_counter = 55;
            spawn_npc(state, NPC_TZ_KIH, 33, 32);
            break;
        case 5:
        case 6:
            state->current_wave = 63;
            spawn_npc(state, NPC_TZTOK_JAD, 45, 32);
            break;
        case 7: {
            static const int types[] = {
                NPC_TZ_KIH, NPC_TZ_KEK, NPC_TZ_KEK_SM, NPC_TOK_XIL,
                NPC_YT_MEJKOT, NPC_KET_ZEK, NPC_TZTOK_JAD, NPC_YT_HURKOT,
            };
            static const int xs[] = {31, 35, 29, 40, 27, 44, 50, 38};
            static const int ys[] = {32, 32, 29, 32, 33, 32, 32, 38};
            state->current_wave = 63;
            for (int i = 0; i < 8; i++) {
                spawn_npc(state, types[i], xs[i], ys[i]);
            }
            break;
        }
        default:
            abort();
    }
    fc_reward_runtime_begin_episode(runtime, state);
}

static int masked_choice(const float* mask, int start, int dim,
                         uint32_t preferred) {
    for (int offset = 0; offset < dim; offset++) {
        int candidate = (int)((preferred + (uint32_t)offset) % (uint32_t)dim);
        if (mask[start + candidate] == 1.0f) return candidate;
    }
    return 0;
}

static void choose_actions(const FcState* state, int trace_id, int tick,
                           uint32_t* action_rng,
                           int actions[FC_NUM_ACTION_HEADS]) {
    float mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(state, mask);
    memset(actions, 0, sizeof(int) * FC_NUM_ACTION_HEADS);
    actions[0] = masked_choice(mask, FC_MASK_MOVE_START, FC_MOVE_DIM,
                               action_next(action_rng));
    actions[1] = masked_choice(mask, FC_MASK_ATTACK_START, FC_ATTACK_DIM,
                               (uint32_t)(1 + tick % FC_VISIBLE_NPCS));

    if (trace_id <= 2) {
        actions[2] = FC_PRAYER_FLICK_MAGIC + trace_id;
    } else if (trace_id == 3) {
        static const int switches[] = {
            FC_PRAYER_MAGIC, FC_PRAYER_RANGE, FC_PRAYER_MELEE, FC_PRAYER_OFF,
        };
        actions[2] = switches[tick % 4];
    } else if (trace_id == 4) {
        actions[2] = tick % REPLAY_FLICK_PERIOD_TICKS == 0
            ? FC_PRAYER_FLICK_MELEE : FC_PRAYER_MELEE;
        if (tick == 8) actions[4] = FC_DRINK_PRAYER_POT;
    } else if (trace_id == 7) {
        actions[2] = tick % FC_PRAYER_DIM;
        if (tick % 29 == 0) {
            actions[5] = 21 + (tick / 29) % 24;
            actions[6] = 21 + (tick / 17) % 24;
        }
    } else {
        actions[2] = tick % 2 == 0 ? FC_PRAYER_FLICK_MAGIC
                                    : FC_PRAYER_FLICK_RANGE;
    }
}

static void print_float_array(FILE* output, const float* values, int count) {
    for (int i = 0; i < count; i++) {
        fprintf(output, "%s%08" PRIx32, i == 0 ? "" : ",",
                float_bits(values[i]));
    }
}

static void print_pending(FILE* output, const FcPendingHit* hits, int count) {
    fprintf(output, "%d", count);
    for (int i = 0; i < count; i++) {
        const FcPendingHit* hit = &hits[i];
        fprintf(output, ",[%d,%d,%d,%d,%d,%d,%d,%d]",
                hit->active, hit->damage, hit->ticks_remaining,
                hit->attack_style, hit->source_npc_idx, hit->prayer_drain,
                hit->prayer_snapshot, hit->prayer_lock_tick);
    }
}

static void print_player(FILE* output, const FcPlayer* player) {
    fprintf(output,
            "x:%d,y:%d,hp:%d/%d,prayer:%d/%d,overhead:%d,start:%d,drain:%d,"
            "timers:%d,%d,%d,%d,run:%d/%d,weapon:%d/%d/%d,ammo:%d,"
            "route:%d/%d,target:%d/%d,events:%d,%d,%d,%d,%d",
            player->x, player->y, player->current_hp, player->max_hp,
            player->current_prayer, player->max_prayer, player->prayer,
            player->prayer_at_tick_start, player->prayer_drain_counter,
            player->attack_timer, player->food_timer, player->potion_timer,
            player->combo_timer, player->run_energy, player->is_running,
            player->weapon_kind, player->weapon_uses_ammo,
            player->crystal_piece_mask, player->ammo_count,
            player->route_len, player->route_idx, player->attack_target_idx,
            player->approach_target, player->damage_taken_this_tick,
            player->hit_style_this_tick, player->hit_locked_prayer_this_tick,
            player->hit_blocked_this_tick, player->hit_landed_this_tick);
}

static void print_npcs(FILE* output, const FcState* state) {
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        const FcNpc* npc = &state->npcs[i];
        const FcNpcStats* stats = fc_npc_get_stats(npc->npc_type);
        int primary_max_hit = fc_npc_max_hit_tenths_for_style(
            stats, stats->attack_style);
        fprintf(output,
                "%s%d:[%d,%d,%d,%d,%d,%d/%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]",
                i == 0 ? "" : ",", i, npc->active, npc->npc_type,
                npc->spawn_index, npc->x, npc->y, npc->current_hp,
                npc->max_hp, npc->is_dead, npc->death_timer,
                npc->attack_style, npc->attack_timer, npc->attack_speed,
                npc->attack_range, primary_max_hit, npc->heal_timer,
                npc->heal_target_idx, npc->num_pending_hits);
    }
}

static void print_runtime(FILE* output, const FcRewardRuntime* runtime) {
    fprintf(output,
            "%d,%d,%08" PRIx32 ",%08" PRIx32 ",%08" PRIx32 ",%08" PRIx32
            ",%08" PRIx32 ",%08" PRIx32 ",%08" PRIx32 ",%08" PRIx32
            ",%08" PRIx32 ",%d,%d,%d,%d",
            runtime->ticks_since_attack, runtime->ticks_in_wave,
            float_bits(runtime->required_work_at_wave_start),
            float_bits(runtime->cave_progress_prev),
            float_bits(runtime->last_required_work_remaining),
            float_bits(runtime->last_current_wave_progress),
            float_bits(runtime->last_cave_progress),
            float_bits(runtime->last_progress_delta),
            float_bits(runtime->last_progress_reward),
            float_bits(runtime->last_net_required_work_removed),
            float_bits((float)runtime->ticks_since_positive_progress),
            runtime->ticks_since_positive_progress,
            runtime->positive_progress_ticks, runtime->zero_progress_ticks,
            runtime->negative_progress_ticks);
}

static void write_tick(FILE* output, int trace_id, int local_tick,
                       const int actions[FC_NUM_ACTION_HEADS],
                       FcState* state, FcRewardRuntime* runtime) {
    float observation[FC_TOTAL_OBS];
    float mask[FC_ACTION_MASK_SIZE];
    float reward_features[FC_REWARD_FEATURES];
    unsigned char native_mask[FC_PUFFER_MASK_SIZE];
    FcRewardParams params = fc_reward_default_params();
    FcRewardBreakdown breakdown =
        fc_reward_compute_breakdown(state, &params, runtime);
    fc_reward_sync_progress_state(state, runtime);

    fc_write_obs(state, observation);
    fc_write_mask(state, mask);
    fc_write_reward_features(state, reward_features);
    for (int i = 0; i < FC_PUFFER_MASK_SIZE; i++) {
        native_mask[i] = (unsigned char)(mask[i] != 0.0f);
    }

    fprintf(output, "tick|trace=%d|index=%d|actions=", trace_id, local_tick);
    for (int i = 0; i < FC_NUM_ACTION_HEADS; i++) {
        fprintf(output, "%s%d", i == 0 ? "" : ",", actions[i]);
    }
    fprintf(output, "|state_hash=%08" PRIx32 "|rng=%08" PRIx32
                    "|terminal=%d|player=",
            fc_state_hash(state), state->rng_state, state->terminal);
    print_player(output, &state->player);
    fprintf(output, "|player_pending=");
    print_pending(output, state->player.pending_hits,
                  state->player.num_pending_hits);
    fprintf(output, "|npcs=");
    print_npcs(output, state);
    fprintf(output, "|observation=");
    print_float_array(output, observation, FC_TOTAL_OBS);
    fprintf(output, "|float_mask=");
    print_float_array(output, mask, FC_ACTION_MASK_SIZE);
    fprintf(output, "|native_mask=");
    for (int i = 0; i < FC_PUFFER_MASK_SIZE; i++) {
        fprintf(output, "%s%u", i == 0 ? "" : ",",
                (unsigned int)native_mask[i]);
    }
    fprintf(output, "|reward_features=");
    print_float_array(output, reward_features, FC_REWARD_FEATURES);
    fprintf(output, "|scalar_reward=%08" PRIx32 "|reward_runtime=",
            float_bits(breakdown.total));
    print_runtime(output, runtime);
    fputc('\n', output);
}

static int run(FILE* output) {
    fprintf(output,
            "meta|backend=%s|schema=1|policy_obs=%d|puffer_obs=%d|"
            "puffer_dims=%d,%d,%d|puffer_mask=%d|core_obs=%d|"
            "core_mask=%d|reward_features=%d|observation_version=%s|"
            "action_version=%s|reward_version=%s|prayer_timing_version=%s|"
            "state_hash_version=%u\n",
            FC_REPLAY_BACKEND_NAME, FC_POLICY_OBS_SIZE, FC_PUFFER_OBS_SIZE,
            FC_PUFFER_ACTION_DIMS[0], FC_PUFFER_ACTION_DIMS[1],
            FC_PUFFER_ACTION_DIMS[2], FC_PUFFER_MASK_SIZE, FC_OBS_SIZE,
            FC_ACTION_MASK_SIZE, FC_REWARD_FEATURES, FC_OBSERVATION_VERSION,
            FC_ACTION_VERSION, FC_REWARD_VERSION, FC_PRAYER_TIMING_VERSION,
            FC_STATE_HASH_VERSION);

    for (int trace_id = 0; trace_id < TRACE_COUNT; trace_id++) {
        uint32_t seed = UINT32_C(0x51f15e00) + (uint32_t)trace_id * 977u;
        uint32_t action_rng = seed ^ UINT32_C(0xa5a5a5a5);
        FcState state;
        FcRewardRuntime runtime;
        fc_init(&state);
        setup_trace(&state, trace_id, seed, &runtime);
        fprintf(output, "scenario|trace=%d|seed=%" PRIu32 "|loadout=%d\n",
                trace_id, seed, state.active_loadout);
        for (int tick = 0; tick < TRACE_TICKS; tick++) {
            int actions[FC_NUM_ACTION_HEADS];
            choose_actions(&state, trace_id, tick, &action_rng, actions);
            fc_step(&state, actions);
            write_tick(output, trace_id, tick, actions, &state, &runtime);
            if (fc_is_terminal(&state)) break;
        }
        fc_destroy(&state);
    }
    return ferror(output) ? 2 : 0;
}

int main(int argc, char** argv) {
    if (argc != 3 || strcmp(argv[1], "--output") != 0) {
        fprintf(stderr, "usage: %s --output PATH\n", argv[0]);
        return 2;
    }
    FILE* output = fopen(argv[2], "w");
    if (output == NULL) {
        fprintf(stderr, "cannot open %s: %s\n", argv[2], strerror(errno));
        return 2;
    }
    int result = run(output);
    if (fclose(output) != 0) {
        fprintf(stderr, "cannot close %s: %s\n", argv[2], strerror(errno));
        return 2;
    }
    return result;
}
