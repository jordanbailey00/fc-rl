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

add_test(NAME phase4_zero_damage_reward
    COMMAND phase2_guardrails_core zero_damage_reward)
set_tests_properties(phase4_zero_damage_reward PROPERTIES
    LABELS "phase4;guardrail")

add_test(NAME phase4_safespot_reward_disabled
    COMMAND phase2_guardrails_core safespot_reward_disabled)
set_tests_properties(phase4_safespot_reward_disabled PROPERTIES
    LABELS "phase4;guardrail")

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

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME phase3_analytics_no_globals
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                analytics_no_global_metrics)
    set_tests_properties(phase3_analytics_no_globals PROPERTIES
        LABELS "phase3;guardrail")

    add_test(NAME phase4_live_safespot_reward_zero
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/validation/tests/phase2_static_guardrails.py
                live_safespot_reward_zero)
    set_tests_properties(phase4_live_safespot_reward_zero PROPERTIES
        LABELS "phase4;guardrail")
else()
    message(STATUS "Python3 not found — skipping phase2 static validation guardrails")
endif()
