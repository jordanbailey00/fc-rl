# fc-validation: focused guardrail tests for environment/training correctness.
# This module consumes fc-core as a library and keeps validation logic out of
# simulator code.

# Performance gates are explicit opt-in targets rather than CTest entries.
# Their wall-clock thresholds are machine-specific, while the executable's
# deterministic workload and machine-readable result are shared by A0 and the
# post-fix comparison.
add_executable(parity_perf_core EXCLUDE_FROM_ALL
    tests/parity_perf_core.c
)
target_include_directories(parity_perf_core PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_perf_core PRIVATE fc_core m)

add_executable(parity_fix_core
    tests/parity_fix_core.c
)
target_include_directories(parity_fix_core PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_core PRIVATE fc_core m)

add_executable(parity_fix_integration
    tests/parity_fix_integration.c
)
target_include_directories(parity_fix_integration PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_integration PRIVATE fc_core m)

add_executable(parity_fix_damage
    tests/parity_fix_damage.c
)
target_include_directories(parity_fix_damage PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_damage PRIVATE fc_core m)

add_executable(parity_fix_prayer
    tests/parity_fix_prayer.c
)
target_include_directories(parity_fix_prayer PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_prayer PRIVATE fc_core m)

add_executable(parity_fix_hash
    tests/parity_fix_hash.c
)
target_include_directories(parity_fix_hash PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_hash PRIVATE fc_core m)

# The same deterministic trace runner is compiled once against the normally
# linked core and once against the exact direct-included source set used by the
# Fight Caves Puffer adapter. The Python comparator owns artifact metadata and
# first-difference diagnostics.
add_executable(parity_fix_replay_linked
    tests/parity_fix_replay.c
)
target_compile_definitions(parity_fix_replay_linked PRIVATE
    FC_REPLAY_BACKEND_NAME="linked_fc_core"
)
target_include_directories(parity_fix_replay_linked PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_replay_linked PRIVATE fc_core m)

add_executable(parity_fix_replay_puffer
    tests/parity_fix_replay.c
    tests/parity_fix_puffer_core.c
)
target_compile_definitions(parity_fix_replay_puffer PRIVATE
    FC_REPLAY_BACKEND_NAME="puffer_direct_include"
)
if(FC_ACTIVE_LOADOUT)
    target_compile_definitions(parity_fix_replay_puffer PRIVATE
        FC_ACTIVE_LOADOUT=${FC_ACTIVE_LOADOUT}
    )
endif()
target_include_directories(parity_fix_replay_puffer PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-training
    ${PROJECT_SOURCE_DIR}/fc-core/include
    ${PROJECT_SOURCE_DIR}/fc-core/src
)
target_link_libraries(parity_fix_replay_puffer PRIVATE m)

add_executable(parity_fix_soak
    tests/parity_fix_soak.c
)
target_include_directories(parity_fix_soak PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_soak PRIVATE fc_core m)

add_executable(parity_fix_rng
    tests/parity_fix_rng.c
)
target_include_directories(parity_fix_rng PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(parity_fix_rng PRIVATE fc_core m)

# Test-only compiled backend used to corrupt each CONTRACT-004 field without
# rebuilding the production CUDA extension for every negative preflight case.
add_library(parity_contract_fixture SHARED
    tests/parity_contract_fixture.c
)

add_library(parity_compiled_contract SHARED
    ${PROJECT_SOURCE_DIR}/fc-training/contract_dump.c
)
target_include_directories(parity_compiled_contract PRIVATE
    ${PROJECT_SOURCE_DIR}/fc-core/include
)

function(add_parity_fix_test name target case_name labels)
    add_test(NAME ${name} COMMAND ${target} ${case_name})
    set_tests_properties(${name} PROPERTIES LABELS "${labels}")
endfunction()

# A2 Workstream 1 NPC/defence assertions.
add_parity_fix_test(parity_npc_001_exact_stat_table
    parity_fix_core npc_001 "parity_fix;unit")
add_parity_fix_test(parity_npc_002_style_maximum_selection
    parity_fix_core npc_002 "parity_fix;unit")
add_parity_fix_test(parity_npc_003_exact_offensive_rolls
    parity_fix_core npc_003 "parity_fix;unit")
add_parity_fix_test(parity_npc_004_real_dual_style_routing
    parity_fix_integration npc_004 "parity_fix;integration")
add_parity_fix_test(parity_npc_005_spawn_and_consumers
    parity_fix_integration npc_005 "parity_fix;integration")
add_parity_fix_test(parity_def_001_sota_exact_rolls
    parity_fix_core def_001 "parity_fix;unit")
add_parity_fix_test(parity_def_002_equipment_field_isolation
    parity_fix_core def_002 "parity_fix;unit")
add_parity_fix_test(parity_def_003_skill_isolation_truncation
    parity_fix_core def_003 "parity_fix;unit")
add_parity_fix_test(parity_def_004_exact_attack_type_propagation
    parity_fix_integration def_004 "parity_fix;integration")

# DEF-005 pins shared accuracy math that is intentionally unchanged and must
# remain part of the non-red preservation run throughout this tranche.
add_parity_fix_test(parity_def_005_hit_chance_preserved
    parity_fix_core def_005 "parity_fix;unit;preservation")

# A2 Workstream 2 base-Ranged/TBow/effect/loadout assertions.
add_parity_fix_test(parity_rng_001_base_loadout_vectors
    parity_fix_core rng_001 "parity_fix;unit;preservation")
add_parity_fix_test(parity_tbow_001_multiplier_vectors
    parity_fix_core tbow_001 "parity_fix;unit")
add_parity_fix_test(parity_tbow_002_exhaustive_integer_oracle
    parity_fix_core tbow_002 "parity_fix;unit")
add_parity_fix_test(parity_tbow_003_effect_gating
    parity_fix_core tbow_003 "parity_fix;unit;preservation")
add_parity_fix_test(parity_cry_001_all_piece_masks
    parity_fix_core cry_001 "parity_fix;unit")
add_parity_fix_test(parity_cry_002_weapon_cross_product
    parity_fix_core cry_002 "parity_fix;unit")
add_parity_fix_test(parity_cry_003_equipment_metadata
    parity_fix_core cry_003 "parity_fix;unit;preservation")
add_parity_fix_test(parity_cry_004_integer_order_bounds
    parity_fix_core cry_004 "parity_fix;unit")
add_parity_fix_test(parity_load_001_exact_raw_values
    parity_fix_core load_001 "parity_fix;unit")
add_parity_fix_test(parity_load_002_weapon_ammo_metadata
    parity_fix_core load_002 "parity_fix;integration;preservation")
add_parity_fix_test(parity_load_003_relationship_checks
    parity_fix_core load_003 "parity_fix;unit")
add_parity_fix_test(parity_load_004_reset_propagation
    parity_fix_core load_004 "parity_fix;integration;preservation")

# A2 Workstream 2 whole-HP damage/tenths-storage assertions.
add_parity_fix_test(parity_dmg_000_unit_explicit_apis
    parity_fix_damage dmg_000 "parity_fix;unit")
add_parity_fix_test(parity_dmg_001_exact_raw_roll_mapping
    parity_fix_damage dmg_001 "parity_fix;unit")
add_parity_fix_test(parity_dmg_002_independent_rng_oracle
    parity_fix_damage dmg_002 "parity_fix;unit;determinism")
add_parity_fix_test(parity_dmg_003_failed_accuracy_behavior
    parity_fix_damage dmg_003 "parity_fix;integration;preservation")
add_parity_fix_test(parity_dmg_004_every_production_call_site
    parity_fix_damage dmg_004 "parity_fix;integration")
add_parity_fix_test(parity_dmg_005_final_maximum_ordering
    parity_fix_damage dmg_005 "parity_fix;unit")
add_parity_fix_test(parity_dmg_006_storage_consumers
    parity_fix_damage dmg_006 "parity_fix;integration;preservation")
add_parity_fix_test(parity_dmg_007_minimum_hit_functional
    parity_fix_damage dmg_007 "parity_fix;integration")
add_parity_fix_test(parity_dmg_008_tz_kih_coupling
    parity_fix_damage dmg_008 "parity_fix;integration;preservation")

# A2 Workstream 3 prayer assertions.
add_parity_fix_test(parity_pray_001_action_ids
    parity_fix_prayer pray_001 "parity_fix;unit")
add_parity_fix_test(parity_pray_002_transition_truth_table
    parity_fix_prayer pray_002 "parity_fix;unit")
add_parity_fix_test(parity_pray_003_drain_decision_table
    parity_fix_prayer pray_003 "parity_fix;unit")
add_parity_fix_test(parity_pray_004_resistance_persistence
    parity_fix_prayer pray_004 "parity_fix;unit")
add_parity_fix_test(parity_pray_005_centralized_depletion
    parity_fix_prayer pray_005 "parity_fix;integration")
add_parity_fix_test(parity_pray_006_tick_start_snapshot
    parity_fix_prayer pray_006 "parity_fix;integration")
add_parity_fix_test(parity_pray_007_repeated_flick_trace
    parity_fix_prayer pray_007 "parity_fix;integration")
add_parity_fix_test(parity_pray_008_jad_delayed_lock
    parity_fix_prayer pray_008 "parity_fix;integration")

# Config workstream 2 observation/deadline semantics.
add_parity_fix_test(parity_obs_001_final_overhead_and_loss
    parity_fix_prayer obs_001 "parity_fix;observation;integration")
add_parity_fix_test(parity_obs_002_final_decision_urgency
    parity_fix_prayer obs_002 "parity_fix;observation;integration")
add_parity_fix_test(parity_obs_003_lock_tick_nonactionable
    parity_fix_prayer obs_003 "parity_fix;observation;unit")
add_parity_fix_test(parity_obs_004_ablation_preserves_deadline
    parity_fix_prayer obs_004 "parity_fix;observation;unit;preservation")

# Config workstream 3 reward-event and metric consumers.
add_parity_fix_test(parity_rwd_001_locked_snapshot_events
    parity_fix_prayer rwd_001 "parity_fix;reward;integration;preservation")
add_parity_fix_test(parity_rwd_002_prayer_loss_sources
    parity_fix_prayer rwd_002 "parity_fix;reward;integration;preservation")
add_parity_fix_test(parity_rwd_003_layout_validity_and_shaping
    parity_fix_prayer rwd_003 "parity_fix;reward;unit;preservation")
add_parity_fix_test(parity_metric_001_tick_start_prayer_uptime
    parity_fix_prayer metric_001 "parity_fix;metrics;unit")

# Config workstream 6 canonical hash assertions.
add_parity_fix_test(parity_det_001_hash_version
    parity_fix_hash version "parity_fix;determinism")
add_parity_fix_test(parity_det_001_field_coverage
    parity_fix_hash field_coverage "parity_fix;determinism")
add_parity_fix_test(parity_det_001_versioned_golden
    parity_fix_hash golden "parity_fix;determinism")
add_parity_fix_test(parity_det_001_presentation_exclusion
    parity_fix_hash presentation "parity_fix;determinism;preservation")

add_test(NAME parity_det_004_player_damage_draw_counts
    COMMAND parity_fix_damage dmg_002)
set_tests_properties(parity_det_004_player_damage_draw_counts PROPERTIES
    LABELS "parity_fix;determinism;rng")

add_test(NAME parity_det_004_npc_attack_draw_counts
    COMMAND parity_fix_integration npc_004)
set_tests_properties(parity_det_004_npc_attack_draw_counts PROPERTIES
    LABELS "parity_fix;determinism;rng")

add_test(NAME parity_det_004_prayer_action_draw_counts
    COMMAND parity_fix_rng)
set_tests_properties(parity_det_004_prayer_action_draw_counts PROPERTIES
    LABELS "parity_fix;determinism;rng")

add_test(NAME parity_soak_001_fast
    COMMAND parity_fix_soak
            --seeds 32
            --ticks 5000
            --seed-start 7001
            --failure-trace ${CMAKE_CURRENT_BINARY_DIR}/parity-soak-001-failure.json)
set_tests_properties(parity_soak_001_fast PROPERTIES
    LABELS "parity_fix;soak;integration")

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

add_test(NAME directional_movement_wall_boundaries
    COMMAND phase2_guardrails_core directional_movement_wall_boundaries)
set_tests_properties(directional_movement_wall_boundaries PROPERTIES
    LABELS "guardrail;movement;collision;osrs")

add_test(NAME directional_movement_diagonal_and_sized
    COMMAND phase2_guardrails_core directional_movement_diagonal_and_sized)
set_tests_properties(directional_movement_diagonal_and_sized PROPERTIES
    LABELS "guardrail;movement;collision;osrs")

add_test(NAME directional_movement_route_regression
    COMMAND phase2_guardrails_core directional_movement_route_regression)
set_tests_properties(directional_movement_route_regression PROPERTIES
    LABELS "guardrail;movement;collision;osrs")

add_test(NAME phase2_npc_moves_when_attack_blocked
    COMMAND phase2_guardrails_core npc_moves_when_attack_blocked)
set_tests_properties(phase2_npc_moves_when_attack_blocked PROPERTIES
    LABELS "phase2;guardrail;movement;combat")

add_test(NAME phase5_invalid_action_classes
    COMMAND phase2_guardrails_core invalid_action_classes)
set_tests_properties(phase5_invalid_action_classes PROPERTIES
    LABELS "phase5;guardrail")

add_test(NAME osrs_hp_regeneration_interval
    COMMAND phase2_guardrails_core hp_regeneration_interval)
set_tests_properties(osrs_hp_regeneration_interval PROPERTIES
    LABELS "guardrail;mechanics;health")

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

add_test(NAME player_directional_ignores_npc_occupancy
    COMMAND phase2_guardrails_core player_directional_ignores_npc_occupancy)
set_tests_properties(player_directional_ignores_npc_occupancy PROPERTIES
    LABELS "guardrail;movement;collision;osrs")

add_test(NAME player_tile_route_accepts_npc_occupied_target
    COMMAND phase2_guardrails_core player_tile_route_accepts_npc_occupied_target)
set_tests_properties(player_tile_route_accepts_npc_occupied_target PROPERTIES
    LABELS "guardrail;movement;collision;osrs")

add_test(NAME player_prebuilt_route_ignores_npc_and_keeps_static_collision
    COMMAND phase2_guardrails_core player_prebuilt_route_ignores_npc_and_keeps_static_collision)
set_tests_properties(player_prebuilt_route_ignores_npc_and_keeps_static_collision PROPERTIES
    LABELS "guardrail;movement;collision;osrs")

add_test(NAME player_move_mask_ignores_npc_and_keeps_static_collision
    COMMAND phase2_guardrails_core player_move_mask_ignores_npc_and_keeps_static_collision)
set_tests_properties(player_move_mask_ignores_npc_and_keeps_static_collision PROPERTIES
    LABELS "guardrail;movement;collision;mask;osrs")

add_test(NAME player_combat_approach_ignores_npc_occupancy
    COMMAND phase2_guardrails_core player_combat_approach_ignores_npc_occupancy)
set_tests_properties(player_combat_approach_ignores_npc_occupancy PROPERTIES
    LABELS "guardrail;movement;collision;combat;osrs")

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

add_test(NAME parity_contract_001_exact_dimensions
    COMMAND phase2_guardrails_training prayer_contract_dimensions)
set_tests_properties(parity_contract_001_exact_dimensions PROPERTIES
    LABELS "parity_fix;contract;action_mask")

add_test(NAME parity_contract_002_puffer_mask_values
    COMMAND phase2_guardrails_training prayer_puffer_mask_values)
set_tests_properties(parity_contract_002_puffer_mask_values PROPERTIES
    LABELS "parity_fix;contract;action_mask")

add_test(NAME parity_contract_003_single_env_canaries
    COMMAND phase2_guardrails_training prayer_single_env_canaries)
set_tests_properties(parity_contract_003_single_env_canaries PROPERTIES
    LABELS "parity_fix;contract;action_mask")

add_test(NAME parity_metric_002_puffer_prayer_publication
    COMMAND phase2_guardrails_training prayer_metric_publication)
set_tests_properties(parity_metric_002_puffer_prayer_publication PROPERTIES
    LABELS "parity_fix;metrics;training")

add_test(NAME cuda_backend_architecture_guardrails
    COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/cuda_arch_guardrails.sh)
set_tests_properties(cuda_backend_architecture_guardrails PROPERTIES
    LABELS "training;guardrail;cuda")

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME parity_det_002_same_version_replay
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_replay.py
                same_version
                $<TARGET_FILE:parity_fix_replay_linked>
                ${CMAKE_CURRENT_BINARY_DIR}/parity-det-002.json)
    set_tests_properties(parity_det_002_same_version_replay PROPERTIES
        LABELS "parity_fix;determinism;replay")

    add_test(NAME parity_det_003_core_puffer_replay
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_replay.py
                core_puffer
                $<TARGET_FILE:parity_fix_replay_linked>
                $<TARGET_FILE:parity_fix_replay_puffer>
                ${CMAKE_CURRENT_BINARY_DIR}/parity-det-003.json)
    set_tests_properties(parity_det_003_core_puffer_replay PROPERTIES
        LABELS "parity_fix;determinism;replay;training")

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

    add_test(NAME parity_contract_002_standalone_action_range
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                standalone_prayer_action_range)
    set_tests_properties(parity_contract_002_standalone_action_range PROPERTIES
        LABELS "parity_fix;contract;action_mask")

    add_test(NAME parity_contract_002_capi_public_header
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                capi_public_batch_contract)
    set_tests_properties(parity_contract_002_capi_public_header PROPERTIES
        LABELS "parity_fix;contract;capi")

    add_test(NAME parity_contract_002_evaluator_dimensions
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_evaluator.py
                contract_dimensions
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_002_evaluator_dimensions PROPERTIES
        LABELS "parity_fix;contract;evaluator")

    add_test(NAME parity_contract_003_evaluator_buffers
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_evaluator.py
                action_and_line_buffers
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_003_evaluator_buffers PROPERTIES
        LABELS "parity_fix;contract;evaluator;buffer")

    add_test(NAME parity_config_001_active_versions
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_config.py
                active_versions)
    set_tests_properties(parity_config_001_active_versions PROPERTIES
        LABELS "parity_fix;config;contract")

    add_test(NAME parity_config_002_runnable_experiment_versions
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_config.py
                runnable_experiment_versions)
    set_tests_properties(parity_config_002_runnable_experiment_versions PROPERTIES
        LABELS "parity_fix;config;contract")

    add_test(NAME parity_config_003_runtime_mirror_versions
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_config.py
                runtime_mirror_versions)
    set_tests_properties(parity_config_003_runtime_mirror_versions PROPERTIES
        LABELS "parity_fix;config;contract")

    add_test(NAME parity_config_004_nonversion_values_preserved
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_config.py
                nonversion_values_preserved)
    set_tests_properties(parity_config_004_nonversion_values_preserved PROPERTIES
        LABELS "parity_fix;config;contract;preservation")

    add_test(NAME parity_contract_004_compiled_preflight
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_preflight.py
                compiled_preflight
                $<TARGET_FILE:parity_compiled_contract>)
    set_tests_properties(parity_contract_004_compiled_preflight PROPERTIES
        LABELS "parity_fix;contract;preflight")

    add_test(NAME parity_contract_004_preflight_rejections
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_preflight.py
                preflight_rejections
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_preflight_rejections PROPERTIES
        LABELS "parity_fix;contract;preflight")

    add_test(NAME parity_contract_004_manifest_schema2
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_preflight.py
                manifest_schema2
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_manifest_schema2 PROPERTIES
        LABELS "parity_fix;contract;manifest")

    add_test(NAME parity_contract_004_consumer_order
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_preflight.py
                consumer_order
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_consumer_order PROPERTIES
        LABELS "parity_fix;contract;preflight;manifest")

    add_test(NAME parity_contract_004_active_configs
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_preflight.py
                active_configs
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_active_configs PROPERTIES
        LABELS "parity_fix;contract;preflight;config")

    add_test(NAME parity_det_001_core_hash_ownership
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_hash_contract.py
                ownership)
    set_tests_properties(parity_det_001_core_hash_ownership PROPERTIES
        LABELS "parity_fix;determinism;contract")

    # Config workstream 8 checkpoint/replay isolation. Explicit random mode
    # and stale-literal scanning already describe preservation behavior; the
    # remaining cases are assertion-level expected-red until the production
    # checkpoint/evaluator correction is approved.
    add_test(NAME parity_contract_004_evaluator_explicit_random
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_checkpoint.py
                evaluator_explicit_random
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_evaluator_explicit_random PROPERTIES
        LABELS "parity_fix;contract;evaluator;preservation")

    add_test(NAME parity_contract_004_stale_literals
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                parity_stale_literals)
    set_tests_properties(parity_contract_004_stale_literals PROPERTIES
        LABELS "parity_fix;contract;static;preservation")

    add_test(NAME parity_contract_004_evaluator_hard_failures
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_checkpoint.py
                evaluator_hard_failures
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_evaluator_hard_failures PROPERTIES
        LABELS "parity_fix;contract;evaluator")

    add_test(NAME parity_contract_004_latest_checkpoint_filter
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_checkpoint.py
                latest_checkpoint_filter
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_latest_checkpoint_filter PROPERTIES
        LABELS "parity_fix;contract;checkpoint")

    add_test(NAME parity_contract_004_manifest_warm_modes
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_checkpoint.py
                manifest_warm_modes
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_manifest_warm_modes PROPERTIES
        LABELS "parity_fix;contract;checkpoint;manifest")

    add_test(NAME parity_contract_004_checkpoint_consumer_order
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_checkpoint.py
                checkpoint_consumer_order
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_contract_004_checkpoint_consumer_order PROPERTIES
        LABELS "parity_fix;contract;checkpoint;evaluator")

    add_test(NAME parity_det_002_trace_provenance
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/parity_fix_checkpoint.py
                trace_provenance
                $<TARGET_FILE:parity_contract_fixture>)
    set_tests_properties(parity_det_002_trace_provenance PROPERTIES
        LABELS "parity_fix;contract;determinism;replay")
else()
    message(STATUS "Python3 not found — skipping phase2 static validation guardrails")
endif()
