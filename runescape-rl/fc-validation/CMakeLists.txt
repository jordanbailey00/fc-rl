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

add_test(NAME phase2_diagonal_corner_clipping
    COMMAND phase2_guardrails_core diagonal_corner_clipping)
set_tests_properties(phase2_diagonal_corner_clipping PROPERTIES
    LABELS "phase2;guardrail;movement")

add_test(NAME phase2_npc_moves_when_attack_blocked
    COMMAND phase2_guardrails_core npc_moves_when_attack_blocked)
set_tests_properties(phase2_npc_moves_when_attack_blocked PROPERTIES
    LABELS "phase2;guardrail;movement;combat")

add_test(NAME phase5_invalid_action_classes
    COMMAND phase2_guardrails_core invalid_action_classes)
set_tests_properties(phase5_invalid_action_classes PROPERTIES
    LABELS "phase5;guardrail")

add_test(NAME phase3_target_identity_stable
    COMMAND phase2_guardrails_core target_identity)
set_tests_properties(phase3_target_identity_stable PROPERTIES
    LABELS "phase3;guardrail")

add_test(NAME option_b_no_move_into_range_fire_same_tick
    COMMAND phase2_guardrails_core option_b_no_move_into_range_fire_same_tick)
set_tests_properties(option_b_no_move_into_range_fire_same_tick PROPERTIES
    LABELS "option_b;guardrail;movement;combat")

add_test(NAME option_b_no_move_into_los_fire_same_tick
    COMMAND phase2_guardrails_core option_b_no_move_into_los_fire_same_tick)
set_tests_properties(option_b_no_move_into_los_fire_same_tick PROPERTIES
    LABELS "option_b;guardrail;movement;combat")

add_test(NAME option_b_attack_then_move_when_already_valid
    COMMAND phase2_guardrails_core option_b_attack_then_move_when_already_valid)
set_tests_properties(option_b_attack_then_move_when_already_valid PROPERTIES
    LABELS "option_b;guardrail;movement;combat")

add_test(NAME option_b_queued_projectile_survives_later_movement
    COMMAND phase2_guardrails_core option_b_queued_projectile_survives_later_movement)
set_tests_properties(option_b_queued_projectile_survives_later_movement PROPERTIES
    LABELS "option_b;guardrail;movement;combat")

add_test(NAME option_b_attack_slot_identity_stable
    COMMAND phase2_guardrails_core target_identity)
set_tests_properties(option_b_attack_slot_identity_stable PROPERTIES
    LABELS "option_b;guardrail;movement;combat")

add_test(NAME step1_movement_only_clears_stale_target
    COMMAND phase2_guardrails_core step1_movement_only_clears_stale_target)
set_tests_properties(step1_movement_only_clears_stale_target PROPERTIES
    LABELS "step1;option_b;guardrail;movement;combat")

add_test(NAME step1_projectile_survives_movement_cancel
    COMMAND phase2_guardrails_core step1_projectile_survives_movement_cancel)
set_tests_properties(step1_projectile_survives_movement_cancel PROPERTIES
    LABELS "step1;option_b;guardrail;movement;combat")

add_test(NAME step1_attack_move_clears_continued_target
    COMMAND phase2_guardrails_core step1_attack_move_clears_continued_target)
set_tests_properties(step1_attack_move_clears_continued_target PROPERTIES
    LABELS "step1;option_b;guardrail;movement;combat")

add_test(NAME step1_attack_move_out_of_range_clears_target
    COMMAND phase2_guardrails_core step1_attack_move_out_of_range_clears_target)
set_tests_properties(step1_attack_move_out_of_range_clears_target PROPERTIES
    LABELS "step1;option_b;guardrail;movement;combat")

add_test(NAME step1_directional_cancels_old_approach_route
    COMMAND phase2_guardrails_core step1_directional_cancels_old_approach_route)
set_tests_properties(step1_directional_cancels_old_approach_route PROPERTIES
    LABELS "step1;option_b;guardrail;movement;combat")

add_test(NAME step1_directional_beats_old_tile_route
    COMMAND phase2_guardrails_core step1_directional_beats_old_tile_route)
set_tests_properties(step1_directional_beats_old_tile_route PROPERTIES
    LABELS "step1;option_b;guardrail;movement;combat")

add_test(NAME step1_attack_approach_without_explicit_movement
    COMMAND phase2_guardrails_core step1_attack_approach_without_explicit_movement)
set_tests_properties(step1_attack_approach_without_explicit_movement PROPERTIES
    LABELS "step1;option_b;guardrail;movement;combat")

add_test(NAME step2_occupancy_marks_and_ignores_entities
    COMMAND phase2_guardrails_core step2_occupancy_marks_and_ignores_entities)
set_tests_properties(step2_occupancy_marks_and_ignores_entities PROPERTIES
    LABELS "step2;guardrail;movement;collision")

add_test(NAME step2_dynamic_footprint_static_and_occupied
    COMMAND phase2_guardrails_core step2_dynamic_footprint_static_and_occupied)
set_tests_properties(step2_dynamic_footprint_static_and_occupied PROPERTIES
    LABELS "step2;guardrail;movement;collision")

add_test(NAME step2_entity_wrapper_ignores_self_blocks_others
    COMMAND phase2_guardrails_core step2_entity_wrapper_ignores_self_blocks_others)
set_tests_properties(step2_entity_wrapper_ignores_self_blocks_others PROPERTIES
    LABELS "step2;guardrail;movement;collision")

add_test(NAME step2_dynamic_diagonal_blocks_occupied_corner
    COMMAND phase2_guardrails_core step2_dynamic_diagonal_blocks_occupied_corner)
set_tests_properties(step2_dynamic_diagonal_blocks_occupied_corner PROPERTIES
    LABELS "step2;guardrail;movement;collision")

add_test(NAME step2_dynamic_bfs_avoids_occupied_steps
    COMMAND phase2_guardrails_core step2_dynamic_bfs_avoids_occupied_steps)
set_tests_properties(step2_dynamic_bfs_avoids_occupied_steps PROPERTIES
    LABELS "step2;guardrail;movement;collision")

add_test(NAME step2_dynamic_sized_bfs_checks_full_footprint
    COMMAND phase2_guardrails_core step2_dynamic_sized_bfs_checks_full_footprint)
set_tests_properties(step2_dynamic_sized_bfs_checks_full_footprint PROPERTIES
    LABELS "step2;guardrail;movement;collision")

add_test(NAME step2_start_reservation_blocks_swap_tile
    COMMAND phase2_guardrails_core step2_start_reservation_blocks_swap_tile)
set_tests_properties(step2_start_reservation_blocks_swap_tile PROPERTIES
    LABELS "step2;guardrail;movement;collision")

add_test(NAME step3_player_directional_blocked_by_npc
    COMMAND phase2_guardrails_core step3_player_directional_blocked_by_npc)
set_tests_properties(step3_player_directional_blocked_by_npc PROPERTIES
    LABELS "step3;guardrail;movement;collision")

add_test(NAME step3_player_tile_route_rejects_occupied_target
    COMMAND phase2_guardrails_core step3_player_tile_route_rejects_occupied_target)
set_tests_properties(step3_player_tile_route_rejects_occupied_target PROPERTIES
    LABELS "step3;guardrail;movement;collision")

add_test(NAME step3_npc_blocked_by_other_npc
    COMMAND phase2_guardrails_core step3_npc_blocked_by_other_npc)
set_tests_properties(step3_npc_blocked_by_other_npc PROPERTIES
    LABELS "step3;guardrail;movement;collision")

add_test(NAME step3_large_npc_blocked_by_healer_footprint
    COMMAND phase2_guardrails_core step3_large_npc_blocked_by_healer_footprint)
set_tests_properties(step3_large_npc_blocked_by_healer_footprint PROPERTIES
    LABELS "step3;guardrail;movement;collision")

add_test(NAME step3_tz_kek_split_avoids_occupied_tiles
    COMMAND phase2_guardrails_core step3_tz_kek_split_avoids_occupied_tiles)
set_tests_properties(step3_tz_kek_split_avoids_occupied_tiles PROPERTIES
    LABELS "step3;guardrail;movement;collision")

add_test(NAME step4_ranged_npc_chases_player_bounds_not_los_tile
    COMMAND phase2_guardrails_core step4_ranged_npc_chases_player_bounds_not_los_tile)
set_tests_properties(step4_ranged_npc_chases_player_bounds_not_los_tile PROPERTIES
    LABELS "step4;guardrail;movement;combat;collision")

add_test(NAME step4_large_npc_chase_checks_full_footprint
    COMMAND phase2_guardrails_core step4_large_npc_chase_checks_full_footprint)
set_tests_properties(step4_large_npc_chase_checks_full_footprint PROPERTIES
    LABELS "step4;guardrail;movement;combat;collision")

add_test(NAME step4_npc_stays_when_current_position_can_attack
    COMMAND phase2_guardrails_core step4_npc_stays_when_current_position_can_attack)
set_tests_properties(step4_npc_stays_when_current_position_can_attack PROPERTIES
    LABELS "step4;guardrail;movement;combat")

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

add_test(NAME revamp_correct_prayer_reward_all_npcs
    COMMAND phase2_guardrails_core correct_prayer_reward_all_npcs)
set_tests_properties(revamp_correct_prayer_reward_all_npcs PROPERTIES
    LABELS "revamp;guardrail;combat;reward")

add_test(NAME revamp_no_attack_penalty_wave_scaled
    COMMAND phase2_guardrails_core no_attack_penalty_wave_scaled)
set_tests_properties(revamp_no_attack_penalty_wave_scaled PROPERTIES
    LABELS "revamp;guardrail")

add_test(NAME simple_reward_player_death_progress_scaled
    COMMAND phase2_guardrails_core player_death_progress_scaled)
set_tests_properties(simple_reward_player_death_progress_scaled PROPERTIES
    LABELS "simple_reward;guardrail;reward")

add_test(NAME simple_reward_zeroed_channels
    COMMAND phase2_guardrails_core simple_reward_zeroed_channels)
set_tests_properties(simple_reward_zeroed_channels PROPERTIES
    LABELS "simple_reward;guardrail;reward")

add_test(NAME npc_kill_reward_eligibility
    COMMAND phase2_guardrails_core npc_kill_reward_eligibility)
set_tests_properties(npc_kill_reward_eligibility PROPERTIES
    LABELS "fight_caves;guardrail;reward;observation")

add_test(NAME tz_kek_split_kill_rewards
    COMMAND phase2_guardrails_core tz_kek_split_kill_rewards)
set_tests_properties(tz_kek_split_kill_rewards PROPERTIES
    LABELS "fight_caves;guardrail;reward;observation")

add_test(NAME wave_clear_reward_scaling
    COMMAND phase2_guardrails_core wave_clear_reward_scaling)
set_tests_properties(wave_clear_reward_scaling PROPERTIES
    LABELS "fight_caves;guardrail;reward;observation")

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

add_test(NAME special_tz_kih_prayer_drain
    COMMAND phase2_guardrails_core special_tz_kih_prayer_drain)
set_tests_properties(special_tz_kih_prayer_drain PROPERTIES
    LABELS "fight_caves;guardrail;combat;special")

add_test(NAME special_mejkot_heal_replaces_attack
    COMMAND phase2_guardrails_core special_mejkot_heal_replaces_attack)
set_tests_properties(special_mejkot_heal_replaces_attack PROPERTIES
    LABELS "fight_caves;guardrail;combat;special")

add_test(NAME special_adjacent_style_selection
    COMMAND phase2_guardrails_core special_adjacent_style_selection)
set_tests_properties(special_adjacent_style_selection PROPERTIES
    LABELS "fight_caves;guardrail;combat;special")

add_test(NAME special_hurkot_behavior
    COMMAND phase2_guardrails_core special_hurkot_behavior)
set_tests_properties(special_hurkot_behavior PROPERTIES
    LABELS "fight_caves;guardrail;combat;special")

add_test(NAME mechanics_observation_events
    COMMAND phase2_guardrails_core mechanics_observation_events)
set_tests_properties(mechanics_observation_events PROPERTIES
    LABELS "fight_caves;guardrail;observation;special")

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

add_test(NAME revamp_native_action_mask_adapter
    COMMAND phase2_guardrails_training native_action_mask_adapter)
set_tests_properties(revamp_native_action_mask_adapter PROPERTIES
    LABELS "revamp;guardrail;action_mask")

add_test(NAME cuda_backend_architecture_guardrails
    COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/cuda_arch_guardrails.sh)
set_tests_properties(cuda_backend_architecture_guardrails PROPERTIES
    LABELS "training;guardrail;cuda")

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

    add_test(NAME revamp_native_action_mask_contract
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                native_action_mask_contract)
    set_tests_properties(revamp_native_action_mask_contract PROPERTIES
        LABELS "revamp;guardrail;action_mask")

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
