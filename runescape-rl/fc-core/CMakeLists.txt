set(FC_CORE_SOURCE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/core_sources.txt)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${FC_CORE_SOURCE_LIST}
)
file(STRINGS ${FC_CORE_SOURCE_LIST} FC_CORE_SOURCES)

add_library(fc_core STATIC ${FC_CORE_SOURCES})
target_include_directories(fc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fc_core PUBLIC m)
set_target_properties(fc_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(FC_ACTIVE_LOADOUT "" CACHE STRING "Optional Fight Caves active loadout override")
if(FC_ACTIVE_LOADOUT)
    target_compile_definitions(fc_core PUBLIC FC_ACTIVE_LOADOUT=${FC_ACTIVE_LOADOUT})
    message(STATUS "Fight Caves active loadout override: ${FC_ACTIVE_LOADOUT}")
endif()
