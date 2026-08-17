#include "fc_actor_visual.h"

#include <math.h>
#include <stdio.h>

static int failures = 0;

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s\n", message); \
        failures++; \
    } \
} while (0)

static void tick_scene(FcVisualScene* scene, int ticks) {
    for (int i = 0; i < ticks; i++)
        fc_visual_scene_update(scene, FC_VISUAL_CLIENT_TICK_SECONDS);
}

static void test_walk_uses_client_local_steps(void) {
    FcVisualScene scene;
    fc_visual_scene_init(&scene);
    fc_visual_scene_reset_player(&scene, 10, 10, 1, 0.0f);
    fc_visual_actor_enqueue_tile(&scene.player, 11, 10, 0);

    tick_scene(&scene, 1);
    CHECK(fabsf(scene.player.local_x - (10.5f * 128.0f + 4.0f)) < 0.01f,
          "walk should advance four local units per client tick");
    tick_scene(&scene, 31);
    CHECK(scene.player.path_count == 0,
          "one walking tile should complete after 32 client ticks");
    CHECK(fabsf(scene.player.local_x - 11.5f * 128.0f) < 0.01f,
          "walking should finish at the destination tile centre");
}

static void test_run_preserves_two_waypoint_route(void) {
    FcVisualScene scene;
    fc_visual_scene_init(&scene);
    fc_visual_scene_reset_player(&scene, 10, 10, 1, 0.0f);
    fc_visual_actor_enqueue_tile(&scene.player, 11, 10, 1);
    fc_visual_actor_enqueue_tile(&scene.player, 12, 10, 1);
    tick_scene(&scene, 32);
    CHECK(scene.player.path_count == 0,
          "two running tiles should complete in 32 client ticks");
    CHECK(fabsf(scene.player.local_x - 12.5f * 128.0f) < 0.01f,
          "run should retain and reach both waypoints");
}

static void test_live_target_turning_and_directional_pose(void) {
    FcVisualScene scene;
    fc_visual_scene_init(&scene);
    fc_visual_scene_reset_player(&scene, 10, 10, 1, 0.0f);
    fc_visual_scene_reset_npc(&scene, 0, 10, 8, 1, 180.0f);
    fc_visual_actor_set_target(&scene.player, FC_VISUAL_TARGET_NPC, 0);
    fc_visual_actor_enqueue_tile(&scene.player, 11, 10, 0);

    tick_scene(&scene, 1);
    CHECK(fabsf(scene.player.yaw_degrees) < 1.0f,
          "player should continue facing a live northern target");
    CHECK(scene.player.locomotion == FC_VISUAL_LOCOMOTION_WALK_RIGHT,
          "eastward movement while facing north should use a side-step pose");

    scene.npcs[0].local_x = 12.5f * 128.0f;
    scene.npcs[0].local_y = 10.5f * 128.0f;
    tick_scene(&scene, 1);
    CHECK(scene.player.yaw_degrees > 0.0f && scene.player.yaw_degrees <= 5.626f,
          "facing should turn gradually toward the target's live position");
}

int main(void) {
    test_walk_uses_client_local_steps();
    test_run_preserves_two_waypoint_route();
    test_live_target_turning_and_directional_pose();
    if (failures) {
        fprintf(stderr, "%d actor visual test(s) failed\n", failures);
        return 1;
    }
    printf("actor visual tests passed\n");
    return 0;
}
