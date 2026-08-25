# fc-viewer: Fight Caves debug viewer.
# Links against the shared fc-core backend and keeps viewer/test utilities local.

set(FC_VIEWER_SOURCES
    src/viewer.c
    src/fc_animated_atlas.c
    src/fc_asset_raylib.c
    src/fc_assets.c
    src/fc_actor_visual.c
    src/fc_click_feedback.c
    src/fc_minimap.c
    src/fc_osrs_text.c
    src/fc_projectile_visual.c
    src/fc_debug.c
    src/ui.c
    src/ui_assets.c
    src/ui_interfaces.c
)

set(RAYLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/raylib" CACHE PATH "Raylib root")

if(EXISTS "${RAYLIB_ROOT}/include/raylib.h")
    add_executable(fc_viewer ${FC_VIEWER_SOURCES})

    target_include_directories(fc_viewer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/fc-core/include
        ${RAYLIB_ROOT}/include
    )

    target_link_libraries(fc_viewer PRIVATE
        fc_core
        ${RAYLIB_ROOT}/lib/libraylib.a
        m dl pthread
    )

    find_package(X11 QUIET)
    if(X11_FOUND)
        target_link_libraries(fc_viewer PRIVATE ${X11_LIBRARIES})
    endif()
    find_package(OpenGL QUIET)
    if(OpenGL_FOUND)
        target_link_libraries(fc_viewer PRIVATE OpenGL::GL)
    endif()

    message(STATUS "Raylib found at ${RAYLIB_ROOT} — fc_viewer will be built")
else()
    message(STATUS "Raylib not found at ${RAYLIB_ROOT} — fc_viewer skipped (set RAYLIB_ROOT)")
endif()

add_executable(test_headless
    tests/test_headless.c
)
target_include_directories(test_headless PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(test_headless PRIVATE fc_core m)
add_test(NAME test_headless COMMAND test_headless)

add_executable(test_actor_visual
    tests/test_actor_visual.c
    src/fc_actor_visual.c
)
target_include_directories(test_actor_visual PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(test_actor_visual PRIVATE m)
add_test(NAME test_actor_visual COMMAND test_actor_visual)

add_executable(test_click_feedback
    tests/test_click_feedback.c
    src/fc_click_feedback.c
)
target_include_directories(test_click_feedback PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
target_link_libraries(test_click_feedback PRIVATE fc_core m)
add_test(NAME test_click_feedback COMMAND test_click_feedback)

if(EXISTS "${RAYLIB_ROOT}/include/raylib.h")
    add_executable(test_minimap
        tests/test_minimap.c
        src/fc_minimap.c
    )
    target_include_directories(test_minimap PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/fc-core/include
        ${RAYLIB_ROOT}/include
    )
    target_link_libraries(test_minimap PRIVATE m)
    add_test(NAME test_minimap COMMAND test_minimap)
endif()

add_executable(test_projectile_visual
    tests/test_projectile_visual.c
    src/fc_projectile_visual.c
)
target_include_directories(test_projectile_visual PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_projectile_visual PRIVATE m)
add_test(NAME test_projectile_visual COMMAND test_projectile_visual)

add_executable(test_anim_mixer
    tests/test_anim_mixer.c
    src/fc_assets.c
)
target_include_directories(test_anim_mixer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_anim_mixer PRIVATE m)
add_test(NAME test_anim_mixer COMMAND test_anim_mixer)

add_executable(validate_assets
    tests/validate_assets.c
    src/fc_assets.c
)
target_include_directories(validate_assets PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/fc-core/include
)
add_test(NAME validate_assets COMMAND validate_assets)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME validate_osrs_text
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/validate_osrs_text.py
    )
endif()
