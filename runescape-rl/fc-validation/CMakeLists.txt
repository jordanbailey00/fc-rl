# fc-validation: focused guardrail tests for environment/training correctness.
# This module consumes fc-core as a library and keeps validation logic out of
# simulator code.

add_executable(phase2_guardrails_core
    tests/phase2_guardrails_core.c
)
target_include_directories(phase2_guardrails_core PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(phase2_guardrails_core PRIVATE fc_core m)

add_test(NAME phase2_safespot_los_preserved
    COMMAND phase2_guardrails_core safespot_los)
set_tests_properties(phase2_safespot_los_preserved PROPERTIES
    LABELS "phase2;guardrail")

add_test(NAME phase5_invalid_action_classes
    COMMAND phase2_guardrails_core invalid_action_classes)
set_tests_properties(phase5_invalid_action_classes PROPERTIES
    LABELS "phase5;guardrail")

add_test(NAME phase3_target_identity_stable
    COMMAND phase2_guardrails_core target_identity)
set_tests_properties(phase3_target_identity_stable PROPERTIES
    LABELS "phase3;guardrail")

add_test(NAME revamp_npc_type_obs_one_hot
    COMMAND phase2_guardrails_core npc_type_obs_one_hot)
set_tests_properties(revamp_npc_type_obs_one_hot PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME phase4_zero_damage_reward
    COMMAND phase2_guardrails_core zero_damage_reward)
set_tests_properties(phase4_zero_damage_reward PROPERTIES
    LABELS "phase4;guardrail")

add_test(NAME phase4_safespot_reward_disabled
    COMMAND phase2_guardrails_core safespot_reward_disabled)
set_tests_properties(phase4_safespot_reward_disabled PROPERTIES
    LABELS "phase4;guardrail")

add_test(NAME revamp_npc_heal_penalty_actual_heal
    COMMAND phase2_guardrails_core npc_heal_penalty_actual_heal)
set_tests_properties(revamp_npc_heal_penalty_actual_heal PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_prayer_loss_penalty
    COMMAND phase2_guardrails_core prayer_loss_penalty)
set_tests_properties(revamp_prayer_loss_penalty PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_no_attack_penalty_wave_scaled
    COMMAND phase2_guardrails_core no_attack_penalty_wave_scaled)
set_tests_properties(revamp_no_attack_penalty_wave_scaled PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_net_progress_required_work
    COMMAND phase2_guardrails_core net_progress_required_work)
set_tests_properties(revamp_net_progress_required_work PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_net_progress_wave_clear
    COMMAND phase2_guardrails_core net_progress_wave_clear)
set_tests_properties(revamp_net_progress_wave_clear PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_net_progress_tz_kek
    COMMAND phase2_guardrails_core net_progress_tz_kek)
set_tests_properties(revamp_net_progress_tz_kek PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_net_progress_jad
    COMMAND phase2_guardrails_core net_progress_jad)
set_tests_properties(revamp_net_progress_jad PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_net_progress_timer_clip
    COMMAND phase2_guardrails_core net_progress_timer_clip)
set_tests_properties(revamp_net_progress_timer_clip PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME revamp_progress_observation_fields
    COMMAND phase2_guardrails_core progress_observation_fields)
set_tests_properties(revamp_progress_observation_fields PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME step3_prayer_deadline_observation_fields
    COMMAND phase2_guardrails_core prayer_deadline_observation_fields)
set_tests_properties(step3_prayer_deadline_observation_fields PROPERTIES
    LABELS "phase3;revamp;guardrail")

add_test(NAME phase3_healer_spawn_validity
    COMMAND phase2_guardrails_core healer_spawn_validity)
set_tests_properties(phase3_healer_spawn_validity PROPERTIES
    LABELS "phase3;guardrail")

add_executable(phase2_guardrails_training
    tests/phase2_guardrails_training.c
)
target_include_directories(phase2_guardrails_training PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-training
    ${PROJECT_SOURCE_DIR}/fc-core/include
    ${PROJECT_SOURCE_DIR}/fc-core/src
)
target_link_libraries(phase2_guardrails_training PRIVATE m)

add_test(NAME phase3_rng_seed_diversity
    COMMAND phase2_guardrails_training rng_seed_diversity)
set_tests_properties(phase3_rng_seed_diversity PROPERTIES
    LABELS "phase3;guardrail")

add_test(NAME phase7_no_supplies_policy_contract
    COMMAND phase2_guardrails_training no_supplies_policy_contract)
set_tests_properties(phase7_no_supplies_policy_contract PROPERTIES
    LABELS "phase7;guardrail")

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME phase3_analytics_no_globals
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                analytics_no_global_metrics)
    set_tests_properties(phase3_analytics_no_globals PROPERTIES
        LABELS "phase3;guardrail")

    add_test(NAME phase7_live_no_supplies_simplified_config
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                live_no_supplies_simplified_config)
    set_tests_properties(phase7_live_no_supplies_simplified_config PROPERTIES
        LABELS "phase7;guardrail")

    add_test(NAME revamp_env_log_key_budget
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                env_log_key_budget)
    set_tests_properties(revamp_env_log_key_budget PROPERTIES
        LABELS "revamp;guardrail")

    add_test(NAME revamp_viewer_policy_pipe_contract
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                viewer_policy_pipe_matches_puffer_contract)
    set_tests_properties(revamp_viewer_policy_pipe_contract PROPERTIES
        LABELS "revamp;guardrail")

    add_test(NAME revamp_viewer_training_parity_contract
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                viewer_training_parity_contract)
    set_tests_properties(revamp_viewer_training_parity_contract PROPERTIES
        LABELS "revamp;guardrail")
else()
    message(STATUS "Python3 not found — skipping phase2 static validation guardrails")
endif()
