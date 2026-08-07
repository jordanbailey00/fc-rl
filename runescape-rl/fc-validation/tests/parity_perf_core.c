#define _POSIX_C_SOURCE 200809L

#include "fc_api.h"
#include "fc_npc.h"
#include "fc_player_init.h"
#include "fc_wave.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

typedef struct {
    uint64_t checksum;
    long long resets;
    long long terminal_resets;
    long long prayer_counts[FC_PRAYER_DIM];
} CorpusResult;

static const int BENCH_WAVES[] = {1, 3, 7, 15, 31, 63};

static uint32_t action_next(uint32_t* state) {
    uint32_t value = *state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    *state = value;
    return value;
}

static double monotonic_seconds(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        perror("clock_gettime");
        exit(2);
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static double timeval_seconds(struct timeval value) {
    return (double)value.tv_sec + (double)value.tv_usec / 1000000.0;
}

static long long parse_positive(const char* text, const char* name) {
    char* end = NULL;
    errno = 0;
    long long value = strtoll(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value <= 0) {
        fprintf(stderr, "invalid %s: %s\n", name, text);
        exit(2);
    }
    return value;
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

static void apply_loadout(FcState* state, int loadout_id) {
    const FcLoadout* loadout = &FC_LOADOUTS[loadout_id];
    FcPlayer* player = &state->player;

    player->max_hp = loadout->max_hp;
    player->current_hp = loadout->max_hp;
    player->max_prayer = loadout->max_prayer;
    player->current_prayer = loadout->max_prayer < 30
        ? loadout->max_prayer : 30;
    player->attack_level = loadout->attack_lvl;
    player->strength_level = loadout->strength_lvl;
    player->defence_level = loadout->defence_lvl;
    player->ranged_level = loadout->ranged_lvl;
    player->prayer_level = loadout->prayer_lvl;
    player->magic_level = loadout->magic_lvl;
    player->weapon_kind = loadout->weapon_kind;
    player->weapon_uses_ammo = loadout->weapon_uses_ammo;
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
#ifdef FC_CRYSTAL_PIECE_ALL
    player->crystal_piece_mask = loadout->crystal_piece_mask;
    state->active_loadout = loadout_id;
#endif
}

static void add_jad_healer(FcState* state) {
    for (int i = 0; i < FC_MAX_NPCS; i++) {
        if (state->npcs[i].active) continue;
        fc_npc_spawn(&state->npcs[i], NPC_YT_HURKOT,
                     22, 22, state->next_spawn_index++);
        state->npcs_remaining++;
        return;
    }
}

static void setup_scenario(FcState* state, uint32_t seed,
                           long long scenario_index) {
    const int wave_count = (int)(sizeof(BENCH_WAVES) / sizeof(BENCH_WAVES[0]));
    int wave = BENCH_WAVES[scenario_index % wave_count];

    fc_reset(state, seed);
    if (wave != 1) {
        memset(state->npcs, 0, sizeof(state->npcs));
        state->npcs_remaining = 0;
        state->current_wave = wave;
        fc_wave_spawn(state, wave);
    }
    if (wave == 63) add_jad_healer(state);

    apply_loadout(state, (scenario_index & 1) != 0
        ? FC_LOADOUT_BOWFA_CRYSTAL : FC_LOADOUT_SOTA_TBOW);
    state->player.prayer = PRAYER_NONE;
    state->player.prayer_drain_counter = 50;
    state->player.sharks_remaining = 0;
    state->player.prayer_doses_remaining = 0;
}

static int masked_choice(const float* mask, int start, int dim,
                         uint32_t preferred) {
    int first_valid = 0;
    for (int offset = 0; offset < dim; offset++) {
        int candidate = (int)((preferred + (uint32_t)offset) % (uint32_t)dim);
        if (mask[start + candidate] > 0.5f) return candidate;
        if (mask[start + offset] > 0.5f) first_valid = offset;
    }
    return first_valid;
}

static CorpusResult run_corpus(long long steps, uint32_t seed,
                               int prayer_limit, long long scenario_span) {
    CorpusResult result = {0};
    FcState state;
    float observation[FC_TOTAL_OBS];
    float mask[FC_ACTION_MASK_SIZE];
    uint32_t action_rng = seed ^ UINT32_C(0x9e3779b9);
    long long scenario_index = 0;
    long long scenario_tick = 0;

    fc_init(&state);
    setup_scenario(&state, seed, scenario_index);
    result.resets = 1;

    for (long long step = 0; step < steps; step++) {
        if (scenario_tick >= scenario_span || fc_is_terminal(&state)) {
            if (fc_is_terminal(&state)) result.terminal_resets++;
            scenario_index++;
            setup_scenario(&state,
                           seed + (uint32_t)(scenario_index * 977u),
                           scenario_index);
            result.resets++;
            scenario_tick = 0;
        }

        fc_write_mask(&state, mask);
        int actions[FC_NUM_ACTION_HEADS] = {0};
        actions[0] = masked_choice(mask, FC_MASK_MOVE_START, FC_MOVE_DIM,
                                   action_next(&action_rng));
        actions[1] = masked_choice(mask, FC_MASK_ATTACK_START, FC_ATTACK_DIM,
                                   action_next(&action_rng));
        actions[2] = masked_choice(mask, FC_MASK_PRAYER_START, prayer_limit,
                                   action_next(&action_rng));
        result.prayer_counts[actions[2]]++;

        fc_step(&state, actions);
        fc_write_obs(&state, observation);

        int obs_index = (int)((uint64_t)step % (uint64_t)FC_TOTAL_OBS);
        uint64_t sample = (uint64_t)(observation[obs_index] * 1000000.0f);
        result.checksum ^= sample + UINT64_C(0x9e3779b97f4a7c15) +
                           (result.checksum << 6) + (result.checksum >> 2);
        result.checksum ^= (uint64_t)(uint32_t)state.rng_state;
        result.checksum += (uint64_t)(uint32_t)(state.current_wave * 131 +
                           state.player.current_hp * 17 +
                           state.npcs_remaining);
        scenario_tick++;
    }

    fc_destroy(&state);
    return result;
}

static void usage(const char* argv0) {
    fprintf(stderr,
        "usage: %s [--warmup-steps N] [--measure-steps N] [--seed N] "
        "[--prayer-limit N] [--scenario-span N]\n", argv0);
}

int main(int argc, char** argv) {
    long long warmup_steps = 250000;
    long long measure_steps = 5000000;
    long long scenario_span = 512;
    uint32_t seed = 73u;
    int prayer_limit = FC_PRAYER_DIM;

    for (int i = 1; i < argc; i++) {
        if (i + 1 >= argc) {
            usage(argv[0]);
            return 2;
        }
        if (strcmp(argv[i], "--warmup-steps") == 0) {
            warmup_steps = parse_positive(argv[++i], "warmup steps");
        } else if (strcmp(argv[i], "--measure-steps") == 0) {
            measure_steps = parse_positive(argv[++i], "measure steps");
        } else if (strcmp(argv[i], "--seed") == 0) {
            seed = parse_seed(argv[++i]);
        } else if (strcmp(argv[i], "--prayer-limit") == 0) {
            prayer_limit = (int)parse_positive(argv[++i], "prayer limit");
        } else if (strcmp(argv[i], "--scenario-span") == 0) {
            scenario_span = parse_positive(argv[++i], "scenario span");
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    if (prayer_limit > FC_PRAYER_DIM) {
        fprintf(stderr, "prayer limit %d exceeds compiled dimension %d\n",
                prayer_limit, FC_PRAYER_DIM);
        return 2;
    }

    (void)run_corpus(warmup_steps, seed, prayer_limit, scenario_span);

    struct rusage usage_before;
    struct rusage usage_after;
    if (getrusage(RUSAGE_SELF, &usage_before) != 0) {
        perror("getrusage");
        return 2;
    }
    double start = monotonic_seconds();
    CorpusResult measured = run_corpus(
        measure_steps, seed, prayer_limit, scenario_span);
    double elapsed = monotonic_seconds() - start;
    if (getrusage(RUSAGE_SELF, &usage_after) != 0) {
        perror("getrusage");
        return 2;
    }

    double cpu_seconds =
        timeval_seconds(usage_after.ru_utime) -
        timeval_seconds(usage_before.ru_utime) +
        timeval_seconds(usage_after.ru_stime) -
        timeval_seconds(usage_before.ru_stime);
    double sps = (double)measure_steps / elapsed;
    double cpu_util_pct = elapsed > 0.0 ? 100.0 * cpu_seconds / elapsed : 0.0;

    printf("PARITY_PERF_JSON={\"kind\":\"core\",\"warmup_steps\":%lld,"
           "\"measured_steps\":%lld,\"scenario_span\":%lld,\"seed\":%u,"
           "\"compiled_prayer_dim\":%d,\"prayer_limit\":%d,"
           "\"observation_floats\":%d,\"mask_floats\":%d,"
           "\"action_heads\":%d,\"state_bytes\":%zu,"
           "\"elapsed_seconds\":%.9f,\"sps\":%.3f,"
           "\"cpu_seconds\":%.9f,\"cpu_util_pct\":%.3f,"
           "\"peak_rss_kb\":%ld,\"resets\":%lld,"
           "\"terminal_resets\":%lld,\"checksum\":\"%016" PRIx64 "\","
           "\"prayer_action_counts\":[",
           warmup_steps, measure_steps, scenario_span, seed,
           FC_PRAYER_DIM, prayer_limit, FC_TOTAL_OBS, FC_ACTION_MASK_SIZE,
           FC_NUM_ACTION_HEADS, sizeof(FcState), elapsed, sps,
           cpu_seconds, cpu_util_pct, usage_after.ru_maxrss,
           measured.resets, measured.terminal_resets, measured.checksum);
    for (int i = 0; i < prayer_limit; i++) {
        printf("%s%lld", i == 0 ? "" : ",", measured.prayer_counts[i]);
    }
    printf("]}\n");
    return 0;
}
