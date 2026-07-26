/*
 * binding.c — PufferLib 4.0 binding for Fight Caves environment.
 *
 * Defines the macros PufferLib needs, includes vecenv.h, and implements
 * the my_init/my_log hooks for config parsing and stat logging.
 */

#include "fight_caves.h"
#include "fc_reward.h"

#define OBS_SIZE FC_PUFFER_OBS_SIZE
#define OBS_TENSOR_T FloatTensor
#define NUM_ATNS FC_PUFFER_NUM_ATNS
#define ACT_SIZES FC_PUFFER_ACT_SIZES
#define OBS_TYPE FLOAT
#define ACT_TYPE DOUBLE
#define MY_ACTION_MASK FC_PUFFER_MASK_SIZE

#define Env FightCaves
#include "vecenv.h"

void my_init(Env* env, Dict* kwargs) {
    env->num_agents = 1;  /* Fight Caves is single-agent */
    FcRewardParams defaults = fc_reward_default_params();

    /* Reward shaping weights (from config/fight_caves.ini [env] section) */
    DictItem* item;
    item = dict_get_unsafe(kwargs, "w_damage_dealt");
    env->w_damage_dealt = item ? (float)item->value : defaults.w_damage_dealt;
    item = dict_get_unsafe(kwargs, "w_progress");
    env->w_progress = item ? (float)item->value : defaults.w_progress;
    item = dict_get_unsafe(kwargs, "negative_progress_multiplier");
    env->negative_progress_multiplier = item ? (float)item->value : defaults.negative_progress_multiplier;
    item = dict_get_unsafe(kwargs, "w_damage_taken");
    env->w_damage_taken = item ? (float)item->value : defaults.w_damage_taken;
    item = dict_get_unsafe(kwargs, "w_npc_kill");
    env->w_npc_kill = item ? (float)item->value : defaults.w_npc_kill;
    item = dict_get_unsafe(kwargs, "w_wave_clear");
    env->w_wave_clear = item ? (float)item->value : defaults.w_wave_clear;
    item = dict_get_unsafe(kwargs, "w_jad_kill");
    env->w_jad_kill = item ? (float)item->value : defaults.w_jad_kill;
    item = dict_get_unsafe(kwargs, "w_cave_complete");
    env->w_cave_complete = item ? (float)item->value : defaults.w_cave_complete;
    item = dict_get_unsafe(kwargs, "w_player_death");
    env->w_player_death = item ? (float)item->value : defaults.w_player_death;
    item = dict_get_unsafe(kwargs, "scale_player_death_with_progress");
    env->scale_player_death_with_progress = item ? (int)item->value : defaults.scale_player_death_with_progress;
    item = dict_get_unsafe(kwargs, "player_death_min_scale");
    env->player_death_min_scale = item ? (float)item->value : defaults.player_death_min_scale;
    item = dict_get_unsafe(kwargs, "w_correct_jad_prayer");
    env->w_correct_jad_prayer = item ? (float)item->value : defaults.w_correct_jad_prayer;
    item = dict_get_unsafe(kwargs, "w_correct_danger_prayer");
    env->w_correct_danger_prayer = item ? (float)item->value : defaults.w_correct_danger_prayer;
    item = dict_get_unsafe(kwargs, "w_prayer_lost");
    env->w_prayer_lost = item ? (float)item->value : defaults.w_prayer_lost;
    item = dict_get_unsafe(kwargs, "w_invalid_action");
    env->w_invalid_action = item ? (float)item->value : defaults.w_invalid_action;
    item = dict_get_unsafe(kwargs, "w_tick_penalty");
    env->w_tick_penalty = item ? (float)item->value : defaults.w_tick_penalty;

    /* Configurable shaping terms */
    item = dict_get_unsafe(kwargs, "shape_unnecessary_prayer_penalty");
    env->shape_unnecessary_prayer_penalty = item ? (float)item->value : defaults.shape_unnecessary_prayer_penalty;
    item = dict_get_unsafe(kwargs, "shape_wave_stall_base_penalty");
    env->shape_wave_stall_base_penalty = item ? (float)item->value : defaults.shape_wave_stall_base_penalty;
    item = dict_get_unsafe(kwargs, "shape_wave_stall_cap");
    env->shape_wave_stall_cap = item ? (float)item->value : defaults.shape_wave_stall_cap;
    item = dict_get_unsafe(kwargs, "shape_jad_heal_penalty");
    env->shape_jad_heal_penalty = item ? (float)item->value : defaults.shape_jad_heal_penalty;
    item = dict_get_unsafe(kwargs, "shape_npc_heal_penalty");
    env->shape_npc_heal_penalty = item ? (float)item->value : defaults.shape_npc_heal_penalty;
    item = dict_get_unsafe(kwargs, "shape_no_progress_penalty_1");
    env->shape_no_progress_penalty_1 = item ? (float)item->value : defaults.shape_no_progress_penalty_1;
    item = dict_get_unsafe(kwargs, "shape_no_progress_penalty_2");
    env->shape_no_progress_penalty_2 = item ? (float)item->value : defaults.shape_no_progress_penalty_2;
    item = dict_get_unsafe(kwargs, "shape_no_progress_penalty_3");
    env->shape_no_progress_penalty_3 = item ? (float)item->value : defaults.shape_no_progress_penalty_3;
    item = dict_get_unsafe(kwargs, "shape_no_attack_base_penalty");
    env->shape_no_attack_base_penalty = item ? (float)item->value : defaults.shape_no_attack_base_penalty;
    item = dict_get_unsafe(kwargs, "shape_no_attack_wave_scale");
    env->shape_no_attack_wave_scale = item ? (float)item->value : defaults.shape_no_attack_wave_scale;
    item = dict_get_unsafe(kwargs, "shape_wave_stall_start");
    env->shape_wave_stall_start = item ? (int)item->value : defaults.shape_wave_stall_start;
    item = dict_get_unsafe(kwargs, "shape_wave_stall_ramp_interval");
    env->shape_wave_stall_ramp_interval = item ? (int)item->value : defaults.shape_wave_stall_ramp_interval;
    item = dict_get_unsafe(kwargs, "shape_no_progress_start_1");
    env->shape_no_progress_start_1 = item ? (int)item->value : defaults.shape_no_progress_start_1;
    item = dict_get_unsafe(kwargs, "shape_no_progress_start_2");
    env->shape_no_progress_start_2 = item ? (int)item->value : defaults.shape_no_progress_start_2;
    item = dict_get_unsafe(kwargs, "shape_no_progress_start_3");
    env->shape_no_progress_start_3 = item ? (int)item->value : defaults.shape_no_progress_start_3;
    item = dict_get_unsafe(kwargs, "shape_no_attack_start");
    env->shape_no_attack_start = item ? (int)item->value : defaults.shape_no_attack_start;
    item = dict_get_unsafe(kwargs, "initial_sharks");
    env->initial_sharks = item ? (int)item->value : 0;
    item = dict_get_unsafe(kwargs, "initial_prayer_doses");
    env->initial_prayer_doses = item ? (int)item->value : 0;

    /* Obs ablation flags (default 0 — i.e. no ablation, full obs).
     * See fc_apply_obs_ablation in fc-core/src/fc_state.c for what each zeroes. */
    item = dict_get_unsafe(kwargs, "obs_ablate_npc_distance");
    env->obs_ablate_npc_distance = item ? (int)item->value : 0;
    item = dict_get_unsafe(kwargs, "obs_ablate_incoming_aggregates");
    env->obs_ablate_incoming_aggregates = item ? (int)item->value : 0;
    item = dict_get_unsafe(kwargs, "obs_ablate_npc_valid");
    env->obs_ablate_npc_valid = item ? (int)item->value : 0;

    /* Initialize game state */
    env->seed_counter = 0;
    fc_init(&env->state);
}

void my_log(Log* log, Dict* out) {
    dict_set(out, "episode_length", log->episode_length);
    dict_set(out, "wave_reached", log->wave_reached);
    dict_set(out, "npcs_slayed", log->npcs_slayed);
    dict_set(out, "prayer_uptime_melee", log->prayer_uptime_melee);
    dict_set(out, "prayer_uptime_range", log->prayer_uptime_range);
    dict_set(out, "prayer_uptime_magic", log->prayer_uptime_magic);
    dict_set(out, "correct_prayer", log->correct_prayer);
    dict_set(out, "wrong_prayer_hits", log->wrong_prayer_hits);
    dict_set(out, "no_prayer_hits", log->no_prayer_hits);
    dict_set(out, "prayer_switches", log->prayer_switches);
    dict_set(out, "damage_blocked", log->damage_blocked);
    dict_set(out, "dmg_taken_avg", log->dmg_taken_avg);
    dict_set(out, "attack_when_ready_rate", log->attack_when_ready_rate);
    dict_set(out, "invalid_move", log->invalid_move);
    dict_set(out, "invalid_attack", log->invalid_attack);
    dict_set(out, "invalid_prayer", log->invalid_prayer);
    dict_set(out, "tokxil_melee_ticks", log->tokxil_melee_ticks);
    dict_set(out, "ketzek_melee_ticks", log->ketzek_melee_ticks);
    dict_set(out, "max_wave_ticks", log->max_wave_ticks);
    dict_set(out, "max_wave_ticks_wave", log->max_wave_ticks_wave);
    dict_set(out, "reached_wave_63", log->reached_wave_63);
    dict_set(out, "jad_kill_rate", log->jad_kill_rate);
    dict_set(out, "player_death_rate", log->player_death_rate);
    dict_set(out, "target_held_ticks", log->target_held_ticks);
    dict_set(out, "no_target_ticks", log->no_target_ticks);
    dict_set(out, "target_in_range_los_ticks", log->target_in_range_los_ticks);
    dict_set(out, "target_out_of_range_or_los_ticks", log->target_out_of_range_or_los_ticks);
    dict_set(out, "attack_cooldown_wait_ticks", log->attack_cooldown_wait_ticks);
    dict_set(out, "ready_but_no_attack_ticks", log->ready_but_no_attack_ticks);
    dict_set(out, "action_move_idle_ticks", log->action_move_idle_ticks);
    dict_set(out, "action_move_walk_ticks", log->action_move_walk_ticks);
    dict_set(out, "action_move_run_ticks", log->action_move_run_ticks);
    dict_set(out, "action_attack_none_ticks", log->action_attack_none_ticks);
    dict_set(out, "action_attack_target_ticks", log->action_attack_target_ticks);
    dict_set(out, "action_prayer_noop_ticks", log->action_prayer_noop_ticks);
    dict_set(out, "action_prayer_cmd_ticks", log->action_prayer_cmd_ticks);
    dict_set(out, "no_progress_ticks", log->no_progress_ticks);
    dict_set(out, "no_progress_idle_move_ticks", log->no_progress_idle_move_ticks);
    dict_set(out, "no_progress_move_cmd_ticks", log->no_progress_move_cmd_ticks);
    dict_set(out, "no_progress_attack_none_ticks", log->no_progress_attack_none_ticks);
    dict_set(out, "no_progress_attack_target_ticks", log->no_progress_attack_target_ticks);
    dict_set(out, "no_progress_has_target_ticks", log->no_progress_has_target_ticks);
    dict_set(out, "no_progress_no_target_ticks", log->no_progress_no_target_ticks);
    dict_set(out, "no_progress_prayer_cmd_ticks", log->no_progress_prayer_cmd_ticks);
    dict_set(out, "no_progress_invalid_action_ticks", log->no_progress_invalid_action_ticks);
    dict_set(out, "required_work_remaining", log->required_work_remaining);
    dict_set(out, "required_work_start", log->required_work_start);
    dict_set(out, "cave_progress", log->cave_progress);
    dict_set(out, "current_wave_progress", log->current_wave_progress);
    dict_set(out, "progress_delta", log->progress_delta);
    dict_set(out, "progress_reward", log->progress_reward);
    dict_set(out, "ticks_since_positive_progress", log->ticks_since_positive_progress);
    dict_set(out, "positive_progress_ticks", log->positive_progress_ticks);
    dict_set(out, "zero_progress_ticks", log->zero_progress_ticks);
    dict_set(out, "negative_progress_ticks", log->negative_progress_ticks);
    dict_set(out, "gross_damage_dealt", log->gross_damage_dealt);
    dict_set(out, "net_required_work_removed", log->net_required_work_removed);
    dict_set(out, "gross_damage_to_net_progress_ratio", log->gross_damage_to_net_progress_ratio);
    dict_set(out, "npc_healing_total", log->npc_healing_total);
    dict_set(out, "mejkot_healing_total", log->mejkot_healing_total);
    dict_set(out, "jad_healing_total", log->jad_healing_total);
    dict_set(out, "preclip_reward", log->preclip_reward);
    dict_set(out, "postclip_reward", log->postclip_reward);
    dict_set(out, "positive_clip_count", log->positive_clip_count);
    dict_set(out, "negative_clip_count", log->negative_clip_count);

    static const char* npc_names[NPC_TYPE_COUNT] = {
        "none",
        "tz_kih",
        "tz_kek",
        "tz_kek_sm",
        "tok_xil",
        "yt_mejkot",
        "ket_zek",
        "tztok_jad",
        "yt_hurkot",
    };
    static char npc_dmg_keys[NPC_TYPE_COUNT][48];
    static char npc_resolved_hit_keys[NPC_TYPE_COUNT][48];
    static char npc_damaging_hit_keys[NPC_TYPE_COUNT][48];
    static char npc_attack_cycle_keys[NPC_TYPE_COUNT][48];
    static char npc_target_tick_keys[NPC_TYPE_COUNT][48];
    static int npc_keys_built = 0;
    if (!npc_keys_built) {
        for (int i = 1; i < NPC_TYPE_COUNT; i++) {
            snprintf(npc_dmg_keys[i], 48, "dmg_to_%s", npc_names[i]);
            snprintf(npc_resolved_hit_keys[i], 48,
                     "resolved_hits_to_%s", npc_names[i]);
            snprintf(npc_damaging_hit_keys[i], 48,
                     "damaging_hits_to_%s", npc_names[i]);
            snprintf(npc_attack_cycle_keys[i], 48,
                     "attack_cycles_to_%s", npc_names[i]);
            snprintf(npc_target_tick_keys[i], 48,
                     "target_ticks_%s", npc_names[i]);
        }
        npc_keys_built = 1;
    }
    for (int i = 1; i < NPC_TYPE_COUNT; i++) {
        dict_set(out, npc_dmg_keys[i], log->dmg_to_npc_type[i]);
        dict_set(out, npc_resolved_hit_keys[i], log->resolved_hits_to_npc_type[i]);
        dict_set(out, npc_damaging_hit_keys[i], log->damaging_hits_to_npc_type[i]);
        dict_set(out, npc_attack_cycle_keys[i], log->attack_cycles_to_npc_type[i]);
        dict_set(out, npc_target_tick_keys[i], log->target_ticks_by_npc_type[i]);
    }

    /* Keys must use static storage because dict_set stores the pointer (see
     * vecenv.h:dict_set); stack-local char arrays would dangle. */
    static char rwd_keys_total[FC_CH_COUNT][48];
    static int rwd_keys_built = 0;
    if (!rwd_keys_built) {
        for (int i = 0; i < FC_CH_COUNT; i++) {
            snprintf(rwd_keys_total[i], 48, "rwd_%s_total", FC_CH_NAMES[i]);
        }
        rwd_keys_built = 1;
    }
    for (int i = 0; i < FC_CH_COUNT; i++) {
        if (i == FC_CH_DAMAGE_DEALT) continue;
        dict_set(out, rwd_keys_total[i], log->rwd_sum[i]);
    }
}
