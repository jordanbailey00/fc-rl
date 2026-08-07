set(FC_CORE_SOURCES
    src/fc_state.c
    src/fc_rng.c
    src/fc_tick.c
    src/fc_combat.c
    src/fc_prayer.c
    src/fc_pathfinding.c
    src/fc_npc.c
    src/fc_wave.c
    src/fc_hash.c
)

add_library(fc_core STATIC ${FC_CORE_SOURCES})
target_include_directories(fc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fc_core PUBLIC m)
set_target_properties(fc_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(FC_ACTIVE_LOADOUT "" CACHE STRING "Optional Fight Caves active loadout override")
if(FC_ACTIVE_LOADOUT)
    target_compile_definitions(fc_core PUBLIC FC_ACTIVE_LOADOUT=${FC_ACTIVE_LOADOUT})
    message(STATUS "Fight Caves active loadout override: ${FC_ACTIVE_LOADOUT}")
endif()
