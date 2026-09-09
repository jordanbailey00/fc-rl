#include "fc_models.h"
#include "raymath.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    /* Tall two-triangle actor. The uploaded mesh deliberately belongs to a
     * different actor: picking must use this instance's pose or the rest mesh. */
    float rest[] = {-0.6f,0,0, 0.6f,0,0, 0.6f,4,0, -0.6f,0,0, 0.6f,4,0, -0.6f,4,0};
    float other_actor[18] = {100,100,100};
    uint16_t faces[] = {0,1,2,0,2,3};
    int16_t pose[] = {-77,0,0, 77,0,0, 77,-512,0, -77,-512,0};
    float original_rest[18]; int16_t original_pose[12];
    memcpy(original_rest, rest, sizeof(rest));
    memcpy(original_pose, pose, sizeof(pose));
    Mesh uploaded = {.vertices=other_actor};
    ModelEntry entry = {.loaded=1, .rest_verts=rest, .face_count=2,
                       .face_indices=faces, .base_vert_count=4};
    entry.model.transform = MatrixIdentity();
    entry.model.meshes = &uploaded;
    Camera3D camera = {.position={0,4,10}, .target={0,2,0}, .up={0,1,0},
                       .fovy=50, .projection=CAMERA_PERSPECTIVE};
    Vector3 origin = {0};
    Vector2 head = GetWorldToScreenEx((Vector3){0,3.8f,0}, camera, 800, 600);
    assert(models_pick_depth(&entry, NULL, origin, 0, camera, head, 800, 600) > 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera, head, 800, 600) > 0);
    /* A head click projects onto ground far behind the actor: the old tile
     * halo would miss, even though the pointer is inside the visible model. */
    Vector3 ray = Vector3Subtract((Vector3){0,3.8f,0}, camera.position);
    float ground_z = camera.position.z + ray.z * (-camera.position.y / ray.y);
    assert(fabsf(ground_z) > 2);

    Vector2 top = GetWorldToScreenEx((Vector3){0,4,0}, camera, 800, 600);
    assert(models_pick_depth(&entry, pose, origin, 0, camera,
        (Vector2){top.x, top.y - 4}, 800, 600) > 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera,
        (Vector2){top.x, top.y - 6}, 800, 600) < 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera, (Vector2){10,10}, 800,600) < 0);

    /* Moving and turned models are picked where drawn, not at their old tile. */
    Vector3 moved = {4,0,0};
    Vector2 moved_head = GetWorldToScreenEx((Vector3){4,3.8f,0}, camera, 800,600);
    assert(models_pick_depth(&entry, pose, moved, 90, camera, moved_head, 800,600) > 0);
    assert(models_pick_depth(&entry, pose, moved, 90, camera, head, 800,600) < 0);
    /* Two instances of the same model can be in different animation poses. */
    int16_t shifted[12]; memcpy(shifted, pose, sizeof(pose));
    for (int i = 0; i < 4; i++) shifted[i*3] += 512;
    assert(models_pick_depth(&entry, shifted, origin, 0, camera, moved_head, 800,600) > 0);
    assert(models_pick_depth(&entry, shifted, origin, 0, camera, head, 800,600) < 0);
    assert(models_pick_depth(&entry, pose, origin, 0, camera, head, 800,600) > 0);

    Vector2 center = GetWorldToScreenEx((Vector3){0,2,0}, camera, 800,600);
    float front = models_pick_depth(&entry, pose, origin, 0, camera, center, 800,600);
    float back = models_pick_depth(&entry, pose, (Vector3){0,0,-4}, 0, camera, center, 800,600);
    assert(front > 0 && back > front); /* overlap ordering */
    assert(models_pick_depth(&entry, pose, (Vector3){0,0,30}, 0, camera, center, 800,600) < 0);
    assert(models_pick_depth(NULL, pose, origin, 0, camera, center, 800,600) < 0);
    assert(memcmp(rest, original_rest, sizeof(rest)) == 0);
    assert(memcmp(pose, original_pose, sizeof(pose)) == 0);
    assert(other_actor[0] == 100 && other_actor[3] == 0);
    puts("model picking: head, padding, rotation, movement, per-instance pose and depth passed");
    return 0;
}
