#include <stdlib.h>

#if defined(_WIN32)
#define FC_TEST_EXPORT __declspec(dllexport)
#else
#define FC_TEST_EXPORT __attribute__((visibility("default")))
#endif

/*
 * Test-only compiled backend fixture for CONTRACT-004. Production preflight
 * must read this symbol from the selected backend; the environment override
 * lets each reported field be corrupted independently without rebuilding the
 * real CUDA extension for every negative case.
 */
FC_TEST_EXPORT const char* fc_training_contract_json(void) {
    const char* override = getenv("FC_TEST_CONTRACT_JSON");
    if (override != NULL && override[0] != '\0') {
        return override;
    }

    return "{"
           "\"contract_dump_schema_version\":1,"
           "\"policy_obs_size\":286,"
           "\"puffer_obs_size\":320,"
           "\"puffer_action_dims\":[17,9,8],"
           "\"puffer_mask_size\":34,"
           "\"core_obs_size\":475,"
           "\"core_action_dims\":[17,9,8,3,2,65,65],"
           "\"core_action_mask\":169,"
           "\"reward_feature_count\":20,"
           "\"observation_version\":\"fight_caves_puffer_policy_obs_v9_run_energy_prayer_timing_mask8_no_supplies\","
           "\"action_version\":\"fight_caves_multidiscrete_3_head_no_supplies_v4_run_energy_prayer8_stationary_attack_tick\","
           "\"reward_version\":\"fight_caves_v4_progress_npc_heal_penalty_m0005_prayer_snapshot_flick_drain\","
           "\"prayer_timing_version\":\"fight_caves_prayer_timing_v1_tick_start_snapshot_flick_drain_jad_lock\","
           "\"state_hash_version\":4,"
           "\"active_loadout\":\"FC_LOADOUT_SOTA_TBOW\""
           "}";
}
