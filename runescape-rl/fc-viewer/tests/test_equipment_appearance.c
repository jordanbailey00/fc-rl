#include "fc_api.h"
#include "fc_items.h"
#include "fc_player_appearance.h"
#include "fc_model_animation.h"
#include <stdio.h>

/* Explicit graphics test, not part of headless CTest. Captures a contact sheet
 * in the working directory: outfit, no helmet/body/legs/gloves/boots/bow, bare. */
int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1200, 640, "Equipment appearance validation");
    if (!IsWindowReady()) return 1;
    FcPlayerAppearance appearance;
    if (!fc_player_appearance_load(&appearance)) return 1;
    AnimCache *cache = anim_cache_load("fc_all.anims");
    if (!cache || !anim_get_sequence(cache, 422)) return 1;
    RenderTexture2D target = LoadRenderTexture(1200, 640);
    BeginTextureMode(target);
    ClearBackground((Color){45, 45, 45, 255});
    const int removed[] = {-1, 0, 4, 7, 9, 10, 3, -2};
    FcState state;
    fc_init(&state);
    for (int variant = 0; variant < 8; variant++) {
        fc_reset(&state, 101);
        fc_set_initial_supplies(&state, 0, 0);
        if (removed[variant] == -2) {
            for (int slot = 0; slot < FC_EQUIPMENT_SLOTS; slot++)
                if (state.player.equipment[slot].item_id &&
                    fc_unequip_item(&state, slot) != FC_ITEM_OK) return 1;
        } else if (removed[variant] >= 0 &&
                   fc_unequip_item(&state, removed[variant]) != FC_ITEM_OK) return 1;
        uint32_t before = fc_state_hash(&state);
        if (fc_player_appearance_sync(&appearance, &state.player, FC_PLAYER_MODEL_BASE) != 1)
            return 1;
        if (fc_player_appearance_sync(&appearance, &state.player, FC_PLAYER_MODEL_BASE) != 0)
            return 1;
        ModelEntry *entry = appearance.model->entries;
        for (int f = 0; f < entry->face_count * 3; f++)
            if (entry->face_indices[f] >= entry->base_vert_count) return 1;
        AnimModelState *pose = NULL;
        uint16_t sequence = 0;
        int frame = 0;
        float timer = 0;
        fc_model_animation_update(entry, cache, &pose, &sequence, &frame,
                                   &timer, 808, 0, 0);
        Camera3D camera = {.position={0, 1.6f, -7}, .target={0, 1.0f, 0},
                           .up={0, 1, 0}, .fovy=4.4f, .projection=CAMERA_ORTHOGRAPHIC};
        BeginScissorMode((variant % 4) * 300, (variant / 4) * 320, 300, 320);
        /* Move the model through one common camera to avoid changing meshes
         * or animation coordinates for the contact-sheet layout. */
        camera.position.x = camera.target.x = ((variant % 4) - 1.5f) * 2.0625f;
        camera.position.y += (variant / 4 ? 1 : -1) * 1.1f;
        camera.target.y += (variant / 4 ? 1 : -1) * 1.1f;
        BeginMode3D(camera);
        DrawModelEx(entry->model, (Vector3){0, 0, 0}, (Vector3){0, 1, 0},
                    180, (Vector3){1, 1, 1}, WHITE);
        EndMode3D();
        EndScissorMode();
        anim_model_state_free(pose);
        if (fc_state_hash(&state) != before) return 1;
        printf("appearance variant %d: %d vertices, %d faces\n",
               variant, entry->base_vert_count, entry->face_count);
    }
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    int ok = ExportImage(image, "equipment-appearance.png");
    UnloadImage(image);
    UnloadRenderTexture(target);
    anim_cache_free(cache);
    fc_player_appearance_free(&appearance);
    CloseWindow();
    return ok ? 0 : 1;
}
