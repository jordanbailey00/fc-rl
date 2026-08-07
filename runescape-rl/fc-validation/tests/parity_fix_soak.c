#include "fc_api.h"
#include "fc_combat.h"
#include "fc_npc.h"
#include "fc_reward.h"
#include "fc_wave.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int* actions;
    uint32_t* hashes;
    int capacity;
    int count;
} Trace;

static uint32_t action_next(uint32_t* state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static int parse_positive(const char* text, const char* name) {
    char* end = NULL;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value <= 0 ||
        value > 100000000L) {
        fprintf(stderr, "invalid %s: %s\n", name, text);
        exit(2);
    }
    return (int)value;
}

static uint32_t parse_seed(const char* text) {
    char* end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value == 0 ||
        value > UINT32_MAX) {
        fprintf(stderr, "invalid seed: %s\n", text);
        exit(2);
    }
    return (uint32_t)value;
}

static void write_failure_trace(const char* path, uint32_t seed,
                                int seed_index, int tick,
                                const char* invariant, const Trace* trace) {
    FILE* output = fopen(path, "w");
    if (output == NULL) {
        fprintf(stderr, "cannot save failing trace %s: %s\n",
                path, strerror(errno));
        return;
    }
    fprintf(output,
            "{\n  \"schema\": 1,\n  \"seed\": %" PRIu32
            ",\n  \"seed_index\": %d,\n  \"failure_tick\": %d,\n"
            "  \"invariant\": \"%s\",\n  \"active_loadout\": %d,\n"
            "  \"state_hash_version\": %u,\n"
            "  \"observation_version\": \"%s\",\n"
            "  \"action_version\": \"%s\",\n"
            "  \"reward_version\": \"%s\",\n"
            "  \"prayer_timing_version\": \"%s\",\n"
            "  \"ticks\": [\n",
            seed, seed_index, tick, invariant, FC_ACTIVE_LOADOUT,
            FC_STATE_HASH_VERSION, FC_OBSERVATION_VERSION, FC_ACTION_VERSION,
            FC_REWARD_VERSION, FC_PRAYER_TIMING_VERSION);
    for (int i = 0; i < trace->count; i++) {
        const int* actions = trace->actions + i * FC_NUM_ACTION_HEADS;
        fprintf(output,
                "    {\"tick\": %d, \"actions\": [%d,%d,%d,%d,%d,%d,%d], "
                "\"state_hash\": \"%08" PRIx32 "\"}%s\n",
                i, actions[0], actions[1], actions[2], actions[3],
                actions[4], actions[5], actions[6], trace->hashes[i],
                i + 1 == trace->count ? "" : ",");
    }
    fprintf(output, "  ]\n}\n");
    fclose(output);
}

static int fail(const char* failure_path, uint32_t seed, int seed_index,
                int tick, const char* invariant, const Trace* trace) {
    fprintf(stderr,
            "FAIL SOAK: seed=%" PRIu32 " seed_index=%d tick=%d: %s\n",
            seed, seed_index, tick, invariant);
    write_failure_trace(failure_path, seed, seed_index, tick, invariant, trace);
    return 1;
}

static int masked_choice(const float* mask, int start, int dim,
                         uint32_t preferred) {
    for (int offset = 0; offset < dim; offset++) {
        int candidate = (int)((preferred + (uint32_t)offset) % (uint32_t)dim);
        if (mask[start + candidate] == 1.0f) return candidate;
    }
    return 0;
}

static void choose_actions(const FcState* state, int tick,
                           uint32_t* action_rng,
                           int actions[FC_NUM_ACTION_HEADS],
                           unsigned* available_attacks,
                           unsigned* selected_attacks,
                           long long prayer_counts[FC_PRAYER_DIM]) {
    float mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(state, mask);
    memset(actions, 0, sizeof(int) * FC_NUM_ACTION_HEADS);

    actions[0] = masked_choice(mask, FC_MASK_MOVE_START, FC_MOVE_DIM,
                               action_next(action_rng));
    for (int attack = 1; attack < FC_ATTACK_DIM; attack++) {
        if (mask[FC_MASK_ATTACK_START + attack] == 1.0f)
            *available_attacks |= 1u << attack;
    }
    actions[1] = masked_choice(mask, FC_MASK_ATTACK_START, FC_ATTACK_DIM,
                               (uint32_t)(1 + tick % FC_VISIBLE_NPCS));
    if (actions[1] > 0) *selected_attacks |= 1u << actions[1];

    actions[2] = tick % FC_PRAYER_DIM;
    prayer_counts[actions[2]]++;

    if (tick % 97 == 0 &&
        mask[FC_MASK_EAT_START + FC_EAT_SHARK] == 1.0f) {
        actions[3] = FC_EAT_SHARK;
    }
    if (tick % 113 == 0 &&
        mask[FC_MASK_DRINK_START + FC_DRINK_PRAYER_POT] == 1.0f) {
        actions[4] = FC_DRINK_PRAYER_POT;
    }
    if (tick % 127 == 0) {
        int x = 1 + (int)(action_next(action_rng) % FC_ARENA_WIDTH);
        int y = 1 + (int)(action_next(action_rng) % FC_ARENA_HEIGHT);
        if (mask[FC_MASK_TARGET_X_START + x] == 1.0f &&
            mask[FC_MASK_TARGET_Y_START + y] == 1.0f) {
            actions[5] = x;
            actions[6] = y;
        }
    }
}

static void setup_seed(FcState* state, uint32_t seed, int seed_index) {
    static const int waves[] = {1, 3, 7, 15, 31, 63};
    int wave = waves[seed_index % (int)(sizeof(waves) / sizeof(waves[0]))];
    fc_reset(state, seed);
    if (wave != 1) {
        memset(state->npcs, 0, sizeof(state->npcs));
        state->npcs_remaining = 0;
        state->current_wave = wave;
        state->next_spawn_index = 0;
        fc_wave_spawn(state, wave);
    }
}

static int pending_valid(const FcState* state, const FcPendingHit* hit,
                         int incoming, const FcNpc* target) {
    if (!hit->active || hit->damage < 0 || hit->damage % 10 != 0 ||
        hit->ticks_remaining < 0 ||
        hit->attack_style < ATTACK_MELEE || hit->attack_style > ATTACK_MAGIC ||
        hit->prayer_snapshot < -1 || hit->prayer_snapshot > PRAYER_PROTECT_MAGIC ||
        hit->prayer_lock_tick < -1) {
        return 0;
    }
    if (incoming) {
        if (hit->source_npc_idx < 0 || hit->source_npc_idx >= FC_MAX_NPCS)
            return 0;
        const FcNpc* source = &state->npcs[hit->source_npc_idx];
        const FcNpcStats* stats = fc_npc_get_stats(source->npc_type);
        int maximum = fc_npc_max_hit_tenths_for_style(stats, hit->attack_style);
        return hit->damage <= maximum;
    }
    if (hit->source_npc_idx != -1 || target == NULL) return 0;
    return hit->damage <=
        fc_player_ranged_final_max_hit_hp(&state->player, target) * 10;
}

static int policy_observation_is_unclamped_nonnegative(int index) {
    if (index == FC_OBS_META_START + FC_OBS_META_DMG_T_TICK) return 1;
    if (index < FC_OBS_NPC_START || index >= FC_OBS_META_START) return 0;
    return (index - FC_OBS_NPC_START) % FC_OBS_NPC_STRIDE ==
        FC_NPC_PENDING_TICKS;
}

static const char* check_invariants(FcState* state, FcRewardRuntime* runtime) {
    const FcPlayer* player = &state->player;
    if (player->max_hp <= 0 || player->current_hp < 0 ||
        player->current_hp > player->max_hp)
        return "player HP outside [0,max]";
    if (player->max_prayer <= 0 || player->current_prayer < 0 ||
        player->current_prayer > player->max_prayer)
        return "player Prayer outside [0,max]";
    if (player->prayer < PRAYER_NONE ||
        player->prayer > PRAYER_PROTECT_MAGIC ||
        player->prayer_at_tick_start < PRAYER_NONE ||
        player->prayer_at_tick_start > PRAYER_PROTECT_MAGIC)
        return "invalid prayer enum";
    if (player->prayer_drain_counter < 0 || player->ammo_count < 0 ||
        player->sharks_remaining < 0 || player->prayer_doses_remaining < 0)
        return "negative resource counter";
    if (player->attack_timer < 0 || player->food_timer < 0 ||
        player->potion_timer < 0 || player->combo_timer < 0 ||
        player->hp_regen_counter < 0)
        return "negative player timer";
    if (player->run_energy < 0 || player->run_energy > 10000 ||
        (player->is_running != 0 && player->is_running != 1))
        return "invalid run state";
    if (player->weapon_kind < FC_WEAPON_GENERIC_RANGED ||
        player->weapon_kind > FC_WEAPON_BOW_OF_FAERDHINEN ||
        (player->weapon_uses_ammo != 0 && player->weapon_uses_ammo != 1) ||
        (player->crystal_piece_mask & ~FC_CRYSTAL_PIECE_ALL) != 0)
        return "invalid weapon/effect metadata";
    if (player->num_pending_hits < 0 ||
        player->num_pending_hits > FC_MAX_PENDING_HITS)
        return "invalid player pending-hit count";
    for (int i = 0; i < player->num_pending_hits; i++) {
        if (!pending_valid(state, &player->pending_hits[i], 1, NULL))
            return "invalid incoming pending hit";
    }
    if (player->damage_taken_this_tick < 0 ||
        player->damage_taken_this_tick % 10 != 0 ||
        state->damage_dealt_this_tick < 0 ||
        state->damage_dealt_this_tick % 10 != 0 ||
        state->damage_taken_this_tick < 0 ||
        state->damage_taken_this_tick % 10 != 0)
        return "damage event is negative or not whole-HP tenths";
    if (state->npcs_remaining < 0 || state->npcs_remaining > FC_MAX_NPCS ||
        state->terminal < TERMINAL_NONE || state->terminal > TERMINAL_TICK_CAP)
        return "invalid NPC count or terminal enum";

    for (int i = 0; i < FC_MAX_NPCS; i++) {
        const FcNpc* npc = &state->npcs[i];
        if (npc->npc_type < NPC_NONE || npc->npc_type >= NPC_TYPE_COUNT ||
            npc->attack_style < ATTACK_NONE || npc->attack_style > ATTACK_MAGIC)
            return "invalid NPC type/style enum";
        if (!npc->active) continue;
        if (npc->max_hp <= 0 || npc->current_hp < 0 ||
            npc->current_hp > npc->max_hp || npc->attack_timer < 0 ||
            npc->death_timer < 0 || npc->heal_timer < 0)
            return "invalid NPC HP/timer";
        if (npc->num_pending_hits < 0 ||
            npc->num_pending_hits > FC_MAX_PENDING_HITS)
            return "invalid NPC pending-hit count";
        if (npc->damage_taken_this_tick < 0 ||
            npc->damage_taken_this_tick % 10 != 0)
            return "NPC damage event is not whole-HP tenths";
        for (int hit = 0; hit < npc->num_pending_hits; hit++) {
            if (!pending_valid(state, &npc->pending_hits[hit], 0, npc))
                return "invalid outgoing pending hit";
        }
    }

    float observation[FC_TOTAL_OBS];
    float mask[FC_ACTION_MASK_SIZE];
    float reward_features[FC_REWARD_FEATURES];
    fc_write_obs(state, observation);
    fc_write_mask(state, mask);
    fc_write_reward_features(state, reward_features);
    for (int i = 0; i < FC_POLICY_OBS_SIZE; i++) {
        if (!isfinite(observation[i]) || observation[i] < -1.0e-6f)
            return "non-finite or out-of-range policy observation";
        if (!policy_observation_is_unclamped_nonnegative(i) &&
            observation[i] > 1.000001f)
            return "bounded policy observation exceeds one";
    }
    for (int i = 0; i < FC_REWARD_FEATURES; i++) {
        if (!isfinite(reward_features[i]))
            return "non-finite reward feature";
    }
    int offset = 0;
    for (int head = 0; head < FC_NUM_ACTION_HEADS; head++) {
        int legal = 0;
        for (int action = 0; action < FC_ACTION_DIMS[head]; action++) {
            float value = mask[offset + action];
            if (value != 0.0f && value != 1.0f)
                return "non-binary action mask";
            legal += value == 1.0f;
        }
        if (legal == 0) return "action head has no legal action";
        offset += FC_ACTION_DIMS[head];
    }
    if (offset != FC_ACTION_MASK_SIZE) return "action mask stride mismatch";

    FcRewardParams params = fc_reward_default_params();
    FcRewardBreakdown breakdown =
        fc_reward_compute_breakdown(state, &params, runtime);
    fc_reward_sync_progress_state(state, runtime);
    if (!isfinite(breakdown.total)) return "non-finite scalar reward";
    return NULL;
}

static int run_seed(uint32_t seed, int seed_index, int tick_budget,
                    const char* failure_path, Trace* trace,
                    unsigned* available_attacks,
                    unsigned* selected_attacks,
                    long long prayer_counts[FC_PRAYER_DIM],
                    long long* total_ticks) {
    FcState state;
    FcRewardRuntime runtime;
    uint32_t action_rng = seed ^ UINT32_C(0x9e3779b9);
    fc_init(&state);
    setup_seed(&state, seed, seed_index);
    fc_reward_runtime_begin_episode(&runtime, &state);
    trace->count = 0;

    for (int tick = 0; tick < tick_budget && !fc_is_terminal(&state); tick++) {
        int* actions = trace->actions + tick * FC_NUM_ACTION_HEADS;
        choose_actions(&state, tick, &action_rng, actions,
                       available_attacks, selected_attacks, prayer_counts);
        fc_step(&state, actions);
        const char* invariant = check_invariants(&state, &runtime);
        trace->hashes[tick] = fc_state_hash(&state);
        trace->count = tick + 1;
        (*total_ticks)++;
        if (invariant != NULL) {
            int result = fail(failure_path, seed, seed_index, tick,
                              invariant, trace);
            fc_destroy(&state);
            return result;
        }
    }

    if (fc_is_terminal(&state)) {
        uint32_t before = fc_state_hash(&state);
        int actions[FC_NUM_ACTION_HEADS] = {0};
        fc_step(&state, actions);
        if (fc_state_hash(&state) != before) {
            int result = fail(failure_path, seed, seed_index, trace->count,
                              "terminal state changed after fc_step", trace);
            fc_destroy(&state);
            return result;
        }
    }
    fc_destroy(&state);
    return 0;
}

int main(int argc, char** argv) {
    int seed_count = 32;
    int tick_budget = 5000;
    uint32_t seed_start = 1;
    const char* failure_path = "parity-soak-failure.json";

    for (int i = 1; i < argc; i++) {
        if (i + 1 >= argc) {
            fprintf(stderr,
                    "usage: %s [--seeds N] [--ticks N] [--seed-start N] "
                    "[--failure-trace PATH]\n", argv[0]);
            return 2;
        }
        if (strcmp(argv[i], "--seeds") == 0) {
            seed_count = parse_positive(argv[++i], "seed count");
        } else if (strcmp(argv[i], "--ticks") == 0) {
            tick_budget = parse_positive(argv[++i], "tick budget");
        } else if (strcmp(argv[i], "--seed-start") == 0) {
            seed_start = parse_seed(argv[++i]);
        } else if (strcmp(argv[i], "--failure-trace") == 0) {
            failure_path = argv[++i];
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return 2;
        }
    }

    Trace trace;
    trace.capacity = tick_budget;
    trace.count = 0;
    trace.actions = calloc((size_t)tick_budget * FC_NUM_ACTION_HEADS,
                           sizeof(*trace.actions));
    trace.hashes = calloc((size_t)tick_budget, sizeof(*trace.hashes));
    if (trace.actions == NULL || trace.hashes == NULL) {
        fprintf(stderr, "cannot allocate replayable soak trace\n");
        free(trace.actions);
        free(trace.hashes);
        return 2;
    }

    unsigned available_attacks = 0;
    unsigned selected_attacks = 0;
    long long prayer_counts[FC_PRAYER_DIM] = {0};
    long long total_ticks = 0;
    for (int seed_index = 0; seed_index < seed_count; seed_index++) {
        uint32_t seed = seed_start + (uint32_t)seed_index * 977u;
        if (seed == 0) seed = UINT32_C(0x12345678);
        if (run_seed(seed, seed_index, tick_budget, failure_path, &trace,
                     &available_attacks, &selected_attacks, prayer_counts,
                     &total_ticks)) {
            free(trace.actions);
            free(trace.hashes);
            return 1;
        }
    }

    for (int action = 0; action < FC_PRAYER_DIM; action++) {
        if (prayer_counts[action] == 0) {
            fprintf(stderr, "FAIL SOAK: prayer action %d was never exercised\n",
                    action);
            free(trace.actions);
            free(trace.hashes);
            return 1;
        }
    }
    if ((available_attacks & selected_attacks) != available_attacks) {
        fprintf(stderr,
                "FAIL SOAK: visible attack slots available=0x%x selected=0x%x\n",
                available_attacks, selected_attacks);
        free(trace.actions);
        free(trace.hashes);
        return 1;
    }

    printf("PASS SOAK: seeds=%d ticks=%lld budget=%d loadout=%d "
           "available_attacks=0x%x selected_attacks=0x%x prayer_counts=",
           seed_count, total_ticks, tick_budget, FC_ACTIVE_LOADOUT,
           available_attacks, selected_attacks);
    for (int action = 0; action < FC_PRAYER_DIM; action++) {
        printf("%s%lld", action == 0 ? "" : ",", prayer_counts[action]);
    }
    putchar('\n');

    free(trace.actions);
    free(trace.hashes);
    return 0;
}
