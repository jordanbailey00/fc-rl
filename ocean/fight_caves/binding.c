/*
 * binding.c — PufferLib 5.0 adapter for Fight Caves: puf_init, puf_reset,
 * puf_step, puf_render, puf_close and puf_log. It is the one C translation
 * unit that compiles the core implementation and the viewer.
 */

/* This translation unit is the sole implementation owner in native builds. */
#define FC_CORE_IMPLEMENTATION
#include "fight_caves.h"
#undef FC_CORE_IMPLEMENTATION

#include <strings.h>

/* Capabilities as built, by [env] key, with the name of each compiled value
 * (an empty name is not a choice). A key that is set must match; a missing
 * one, as in a config-less viewer or a test, takes the built value. */
#define FC_CAPABILITY_CHOICES 5
static const struct {
    const char* key;
    int built;
    int numeric;   /* compared as the number PufferLib parsed: 1, 1.0, true */
    const char* names[FC_CAPABILITY_CHOICES];
} FC_BUILT_CAPABILITIES[] = {
    {"consumables", FC_CAP_CONSUMABLES, 1, {"0", "1"}},
    {"prayers", FC_CAP_PRAYERS, 0, {"protection", "all"}},
    {"gear_sets", FC_CAP_GEAR_SETS, 1, {"", "1", "2", "3", "4"}},
    {"spellbook", FC_CAP_SPELLBOOK, 0, {"none", "standard", "ancient"}},
};
#define FC_BUILT_CAPABILITY_COUNT \
    ((int)(sizeof(FC_BUILT_CAPABILITIES) / sizeof(FC_BUILT_CAPABILITIES[0])))

/* The [env] settings puf_init and the viewer's manual play share: every
 * FC_REWARD_SETTINGS key (a missing key keeps its default), the capabilities,
 * which must match this build, and the scenario. Returns 0, or -1 with the
 * reason in err. */
static int fc_read_env_config(Dict* kwargs, FcRewardParams* reward,
                              FcScenario* scenario, char* err, size_t err_size) {
    *reward = fc_reward_default_params();
    for (int i = 0; i < FC_REWARD_SETTING_COUNT; i++) {
        DictItem* item = dict_find(kwargs, FC_REWARD_SETTINGS[i].key);
        if (item != NULL)
            fc_reward_setting_store(reward, &FC_REWARD_SETTINGS[i], item->value);
    }
    for (int i = 0; i < FC_BUILT_CAPABILITY_COUNT; i++) {
        const char* key = FC_BUILT_CAPABILITIES[i].key;
        const char* const* names = FC_BUILT_CAPABILITIES[i].names;
        DictItem* item = dict_find(kwargs, key);
        if (item == NULL) continue;
        char number[32];
        const char* text = item->str;
        if (FC_BUILT_CAPABILITIES[i].numeric || text == NULL) {
            snprintf(number, sizeof(number), "%g", item->value);
            text = number;
        }
        int value = -1;
        for (int v = 0; v < FC_CAPABILITY_CHOICES && names[v]; v++)
            if (names[v][0] && strcasecmp(text, names[v]) == 0) value = v;
        if (value < 0) {
            int n = snprintf(err, err_size, "%s = %s: must be", key,
                             item->str ? item->str : text);
            for (int v = 0, listed = 0; v < FC_CAPABILITY_CHOICES && names[v] &&
                 n > 0 && (size_t)n < err_size; v++)
                if (names[v][0])
                    n += snprintf(err + n, err_size - n, "%s %s",
                                  listed++ ? " or" : "", names[v]);
            return -1;
        }
        if (value != FC_BUILT_CAPABILITIES[i].built) {
            snprintf(err, err_size,
                     "%s = %s, but this build has %s = %s. Capabilities change the "
                     "action heads and observation, so set them in "
                     "config/fight_caves.ini and rebuild: ./build.sh fight_caves",
                     key, names[value], key, names[FC_BUILT_CAPABILITIES[i].built]);
            return -1;
        }
    }

    *scenario = fc_default_scenario();
    DictItem* item;
    for (int k = 0; k < FC_MAX_GEAR_SETS; k++) {
        char key[32];
        snprintf(key, sizeof(key), "gear_set_%d", k + 1);
        item = dict_find(kwargs, key);
        if (item == NULL) continue;
        const char* name = item->str ? item->str : "";
        scenario->gear_set[k] = fc_gear_preset_find(name);
        if (scenario->gear_set[k] < 0) {
            int n = snprintf(err, err_size, "%s = %s: no such gear preset; "
                             "the presets are", key, name);
            for (int i = 0; i < FC_GEAR_PRESET_COUNT && n > 0 && (size_t)n < err_size; i++)
                n += snprintf(err + n, err_size - n, " %s", FC_GEAR_PRESETS[i].name);
            return -1;
        }
    }
    for (int s = 0; s < FC_SKILL_COUNT; s++) {
        char key[32];
        snprintf(key, sizeof(key), "%s_level", FC_SKILL_NAMES[s]);
        item = dict_find(kwargs, key);
        if (item != NULL) scenario->levels[s] = (int)item->value;
    }
    item = dict_find(kwargs, "initial_sharks");
    scenario->sharks = item ? (int)item->value : 0;
    item = dict_find(kwargs, "initial_prayer_doses");
    scenario->prayer_doses = item ? (int)item->value : 0;
    /* Supplies are clamped to what the inventory holds. */
    scenario->sharks = scenario->sharks < 0 ? 0
        : scenario->sharks > FC_MAX_SHARKS ? FC_MAX_SHARKS : scenario->sharks;
    scenario->prayer_doses = scenario->prayer_doses < 0 ? 0
        : scenario->prayer_doses > FC_MAX_PRAYER_DOSES ? FC_MAX_PRAYER_DOSES
        : scenario->prayer_doses;
    return fc_scenario_check(scenario, err, err_size);
}

/* fc_read_env_config, stopping with the reason on a bad config. */
static void fc_apply_env_config(Dict* kwargs, FcRewardParams* reward,
                                FcScenario* scenario) {
    char err[512];
    if (fc_read_env_config(kwargs, reward, scenario, err, sizeof(err)) != 0) {
        fprintf(stderr, "Fight Caves config error: %s\n", err);
        exit(EXIT_FAILURE);
    }
}

/* What this build's policy sees and does, and what an episode starts with. */
static void fc_print_summary(FILE* out, const FcScenario* scenario) {
    fprintf(out, "Fight Caves:");
    for (int i = 0; i < FC_BUILT_CAPABILITY_COUNT; i++)
        fprintf(out, " %s=%s", FC_BUILT_CAPABILITIES[i].key,
                FC_BUILT_CAPABILITIES[i].names[FC_BUILT_CAPABILITIES[i].built]);
    fprintf(out, " | heads");
    for (int h = 0; h < FC_PUFFER_NUM_ATNS; h++)
        fprintf(out, "%s %s %d", h ? "," : "",
                FC_HEAD_NAMES[FC_PUFFER_ACTION_HEADS[h]], FC_PUFFER_ACTION_DIMS[h]);
    fprintf(out, " | obs %d + %d mask = %d\n", FC_POLICY_OBS_SIZE,
            FC_PUFFER_MASK_SIZE, FC_PUFFER_OBS_SIZE);
    fprintf(out, "Fight Caves scenario:");
    for (int k = 0; k < FC_CAP_GEAR_SETS; k++) {
        const FcGearPreset* preset = &FC_GEAR_PRESETS[scenario->gear_set[k]];
        fprintf(out, "%s gear_set_%d = %s", k ? "," : "", k + 1, preset->name);
        if (preset->autocast)
            fprintf(out, " (spell %s)", FC_SPELLS[preset->autocast - 1].name);
    }
    fprintf(out, " | levels");
    for (int s = 0; s < FC_SKILL_COUNT; s++)
        fprintf(out, "%s %s %d", s ? "," : "", FC_SKILL_NAMES[s], scenario->levels[s]);
    fprintf(out, " | %d sharks, %d prayer doses\n", scenario->sharks,
            scenario->prayer_doses);
}

int fc_describe_env(Dict* kwargs) {
    FcRewardParams reward;
    FcScenario scenario;
    char err[512];
    if (fc_read_env_config(kwargs, &reward, &scenario, err, sizeof(err)) != 0) {
        fprintf(stderr, "Fight Caves config error: %s\n", err);
        return 1;
    }
    fc_print_summary(stdout, &scenario);
    return 0;
}

#include "viewer.c"

static void fc_puffer_accumulate_episode_summary(
    Log* log, const FcEpisodeSummary* summary) {
    log->zero_progress_ticks += (float)summary->zero_progress_ticks;
    log->wave_reached += (float)summary->wave_reached;
    log->wrong_prayer_hits += (float)summary->wrong_prayer_hits;
    log->reached_wave_63 += (float)summary->reached_wave_63;
    log->jad_kill_rate += (float)summary->jad_kill_rate;
    log->prayer_uptime_range += (float)summary->prayer_uptime_range;
    log->prayer_uptime_melee += (float)summary->prayer_uptime_melee;
    log->prayer_uptime_magic += (float)summary->prayer_uptime_magic;
    log->npc_healing_total += (float)summary->npc_healing_total;
    log->jad_healing_total += (float)summary->jad_healing_total;
    log->episode_length += (float)summary->episode_length;
}

static void fc_puffer_write_obs(FightCaves* env) {
    Agent* agent = &env->agents[0];
    float* obs = agent->observations;

    /* Policy observations */
    fc_write_obs(&env->state, obs);

    /* Keep the float mask of the policy heads in observations for checkpoint
     * compatibility, and publish the same legality flags through PufferLib's
     * native mask channel. */
    float full_mask[FC_ACTION_MASK_SIZE];
    fc_write_mask(&env->state, full_mask);
    float* mask = obs + FC_POLICY_OBS_SIZE;
    for (int h = 0; h < FC_PUFFER_NUM_ATNS; h++) {
        memcpy(mask, full_mask + FC_MASK_HEAD_START[FC_PUFFER_ACTION_HEADS[h]],
               sizeof(float) * FC_PUFFER_ACTION_DIMS[h]);
        mask += FC_PUFFER_ACTION_DIMS[h];
    }
    if (agent->action_mask != NULL) {
        for (int i = 0; i < FC_PUFFER_MASK_SIZE; i++) {
            agent->action_mask[i] =
                (unsigned char)(obs[FC_POLICY_OBS_SIZE + i] != 0.0f);
        }
    }
}

/* ======================================================================== */
/* Reward computation                                                        */
/* ======================================================================== */

static float fc_puffer_compute_reward(FightCaves* env) {
    FcRewardBreakdown breakdown = fc_reward_compute_breakdown(
        &env->state, &env->reward_params, &env->reward_runtime);
    if (env->viewer) env->viewer->pending_reward_breakdown = breakdown;
    fc_reward_sync_progress_state(&env->state, &env->reward_runtime);
    return breakdown.total;
}


/* ======================================================================== */
/* PufferLib interface: puf_reset, puf_step, puf_render, puf_close           */
/* ======================================================================== */

static uint32_t fc_puffer_mix_reset_seed(uint32_t env_rng, uint32_t episode) {
    uint32_t x = env_rng + 0x9E3779B9u * (episode + 1u);
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return (x != 0u) ? x : 0x12345678u;
}

void puf_reset(Env* env) {
    env->seed_counter++;
    fc_reset(&env->state,
             fc_puffer_mix_reset_seed((uint32_t)env->rng, env->seed_counter));
    fc_apply_scenario(&env->state, &env->scenario);
    fc_reward_runtime_begin_episode(&env->reward_runtime, &env->state);
    /* Compute initial observations */
    fc_puffer_write_obs(env);
    if (env->viewer) env->viewer->reset_state = env->state;
}

void puf_step(Env* env) {
    Agent* agent = &env->agents[0];
    agent->terminals[0] = 0.0f;

    /* PufferLib sends one float per policy head. The core heads the policy
     * does not emit stay zero, their no-op: no walk-to-tile, and no eating
     * or drinking without the consumables capability. */
    int actions[FC_NUM_ACTION_HEADS] = {0};
    for (int h = 0; h < FC_PUFFER_NUM_ATNS; h++) {
        actions[FC_PUFFER_ACTION_HEADS[h]] = (int)agent->actions[h];
    }

    /* Step the game simulation */
    fc_step(&env->state, actions);

    /* Compute reward */
    agent->rewards[0] = fc_puffer_compute_reward(env);

    /* Write the current tick's observation. On terminal steps, puf_reset()
     * below replaces it with the next episode's initial observation. */
    fc_puffer_write_obs(env);

    /* A value snapshot survives same-step autoreset. Worker threads only copy
     * data here; all graphics and frame pacing remain inside puf_render(). */
    if (env->viewer) {
        env->viewer->pending_state = env->state;
        env->viewer->pending_reward_runtime = env->reward_runtime;
        memcpy(env->viewer->pending_actions, actions, sizeof(actions));
        env->viewer->pending_frame = 1;
    }

    /* Check terminal */
    if (fc_is_terminal(&env->state)) {
        FcEpisodeSummary summary;
        fc_episode_summary_build(&env->state, &env->reward_runtime,
                                 env->state.tick, &summary);
        agent->terminals[0] = 1.0f;
        fc_puffer_accumulate_episode_summary(&env->log, &summary);
        env->log.n += 1.0f;

        /* Same-step autoreset: return the completed episode's reward and
         * terminal flag alongside the next episode's initial observation. */
        puf_reset(env);
    }
}

void puf_render(Env* env) {
    if (!env->viewer) {
        env->viewer = fc_viewer_create(1);
        if (!env->viewer) exit(EXIT_FAILURE);
        ViewerState* v = env->viewer;
        v->state = v->reset_state = env->state;
        v->reward_params = env->reward_params;
        v->scenario = env->scenario;
        v->reward_runtime = env->reward_runtime;
        snprintf(v->reward_config_path, sizeof(v->reward_config_path),
                 "Puffer environment configuration");
        v->reward_config_loaded = 1;
        fc_viewer_reset_presentation(v);
    }
    ViewerState* v = env->viewer;
    if (!v->pending_frame &&
        (v->state.rng_seed != env->state.rng_seed ||
         v->state.tick != env->state.tick)) {
        /* Also support an explicit puf_reset() between render calls. */
        v->state = env->state;
        v->reward_runtime = env->reward_runtime;
        memset(&v->reward_breakdown, 0, sizeof(v->reward_breakdown));
        fc_viewer_reset_presentation(v);
    }
    fc_viewer_present_pending(v);
    int frame;
    do {
        frame = fc_viewer_frame(env->viewer);
    } while (frame == 0);
    if (frame < 0) {
        fc_viewer_destroy(env->viewer);
        env->viewer = NULL;
        exit(EXIT_SUCCESS);
    }
}

void puf_close(Env* env) {
    fc_viewer_destroy(env->viewer);
    env->viewer = NULL;
    fc_destroy(&env->state);
}

void puf_init(Env* env, Dict* kwargs) {
    env->num_agents = 1;  /* Fight Caves is single-agent */
    fc_apply_env_config(kwargs, &env->reward_params, &env->scenario);
    /* Puffer initializes every env on one thread; print the summary once. */
    static int summary_printed = 0;
    if (!summary_printed) {
        fc_print_summary(stdout, &env->scenario);
        summary_printed = 1;
    }

    /* Callers pass a zeroed Env (Puffer callocs its envs and zeroes its
     * bufferless probe), so the other fields need no initialization. */
    fc_init(&env->state);
}

void puf_log(Log* log, Dict* out) {
    /* Native evaluation requires perf; use the existing success metric for
     * both standard aliases without adding counters or changing rewards. */
    dict_set(out, "perf", log->jad_kill_rate);
    dict_set(out, "score", log->jad_kill_rate);
    dict_set(out, "zero_progress_ticks", log->zero_progress_ticks);
    dict_set(out, "wave_reached", log->wave_reached);
    dict_set(out, "wrong_prayer_hits", log->wrong_prayer_hits);
    dict_set(out, "reached_wave_63", log->reached_wave_63);
    dict_set(out, "jad_kill_rate", log->jad_kill_rate);
    dict_set(out, "prayer_uptime_range", log->prayer_uptime_range);
    dict_set(out, "prayer_uptime_melee", log->prayer_uptime_melee);
    dict_set(out, "prayer_uptime_magic", log->prayer_uptime_magic);
    dict_set(out, "npc_healing_total", log->npc_healing_total);
    dict_set(out, "jad_healing_total", log->jad_healing_total);
    dict_set(out, "episode_length", log->episode_length);
}
