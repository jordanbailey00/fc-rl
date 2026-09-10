#include "fc_api.h"
#include "fc_items.h"
#include "fc_player_appearance.h"
#include "fc_model_animation.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "cape check failed at %d: %s\n", __LINE__, #condition); \
    return 0; } } while (0)

/* Rear views are essential: front-only outfit checks cannot see cape materials.
 * Render the same idle/walk/attack poses at two texture times. */
static int check_capes(FcPlayerAppearance *appearance, AnimCache *cache, int draw_mode) {
    const int kits[] = {FC_LOADOUT_MELEE_HIGH, FC_LOADOUT_MELEE_MAX,
                        FC_LOADOUT_MAGIC_HIGH, FC_LOADOUT_MAGIC_LOW};
    const int capes[] = {6570, 21295, 21791, 2412};
    const int textured_faces[] = {24, 30, 0, 0};
    const int animations[] = {808, 819, 422};
    CHECK(appearance->atlas.anim_count == 3); /* Also includes the imbued shortbow's texture 34. */
    CHECK(appearance->atlas.anims[1].texture_id == 40);
    CHECK(appearance->atlas.anims[1].direction == 1 && appearance->atlas.anims[1].speed == 2);
    CHECK(appearance->atlas.anims[2].texture_id == 59);
    CHECK(appearance->atlas.anims[2].direction == 1 && appearance->atlas.anims[2].speed == 1);
    RenderTexture2D target = LoadRenderTexture(1600, 1200);
    for (int time = 0; time < 2; time++) {
        fc_animated_atlas_update(&appearance->atlas, time ? 0.2f : 0);
        if (time) CHECK(memcmp(appearance->atlas.pixels, appearance->atlas.base_pixels,
                               (size_t)appearance->atlas.width * appearance->atlas.height * 4) != 0);
        BeginTextureMode(target);
        ClearBackground((Color){45, 45, 45, 255});
        for (int kit = 0; kit < 4; kit++) {
            FcState state;
            fc_init(&state);
            fc_reset(&state, 101);
            fc_set_initial_supplies(&state, 0, 0);
            CHECK(fc_apply_loadout(&state, kits[kit]) == FC_ITEM_OK);
            uint32_t before = fc_state_hash(&state);
            CHECK(fc_player_appearance_sync(appearance, &state.player, FC_PLAYER_MODEL_BASE) == 1);
            ModelEntry *entry = appearance->model->entries;
            ModelEntry *cape = model_find(appearance->parts, (uint32_t)capes[kit]);
            CHECK(cape && appearance->model->has_textures && entry->face_uvs);
            CHECK(entry->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.id ==
                  appearance->atlas.texture.id);
            /* The composer must rebase the texture triangle as well as the mesh
             * triangles. Body kits and the hat precede the cape. */
            uint32_t hidden = 0;
            for (int slot = 0; slot < FC_EQUIPMENT_SLOTS; slot++)
                for (int r = 0; r < appearance->record_count; r++)
                    if (appearance->records[r].item_id == (uint32_t)state.player.equipment[slot].item_id)
                        hidden |= appearance->records[r].hide_mask;
            int vo = 0, fo = 0;
            for (int body = 0; body < 7; body++) {
                if (hidden & (1u << body)) continue;
                ModelEntry *part = model_find(appearance->parts, 0xFC100000u + (uint32_t)body);
                vo += part->base_vert_count;
                fo += part->face_count;
            }
            ModelEntry *hat = model_find(appearance->parts, (uint32_t)state.player.equipment[0].item_id);
            CHECK(hat);
            vo += hat->base_vert_count;
            fo += hat->face_count;
            int textured = 0;
            for (int f = 0; f < cape->face_count; f++) {
                const ModelFaceUvInfo *src = &cape->face_uvs[f], *dst = &entry->face_uvs[fo + f];
                CHECK(src->textured == dst->textured);
                if (!src->textured) continue;
                textured++;
                CHECK(dst->tex_a == src->tex_a + vo && dst->tex_b == src->tex_b + vo &&
                      dst->tex_c == src->tex_c + vo);
                for (int j = 0; j < 3; j++)
                    for (int c = 0; c < 3; c++)
                        CHECK(cape->model.meshes[0].colors[(f * 3 + j) * 4 + c] == 255);
            }
            CHECK(textured == textured_faces[kit]);
            if (kit == 2) {
                unsigned int total = 0;
                for (int v = 0; v < cape->face_count * 3; v++)
                    for (int c = 0; c < 3; c++) total += cape->model.meshes[0].colors[v * 4 + c];
                CHECK(total / (cape->face_count * 9) > 100); /* Not the old near-black bake. */
            }
            for (int pose_id = 0; pose_id < 3; pose_id++) {
                CHECK(anim_get_sequence(cache, (uint16_t)animations[pose_id]));
                AnimModelState *pose = NULL;
                uint16_t sequence = 0;
                int frame = 0;
                float timer = 0;
                fc_model_animation_update(entry, cache, &pose, &sequence, &frame,
                                           &timer, animations[pose_id], 0, 2);
                CHECK(pose && entry->model.meshes[0].texcoords);
                for (int f = 0; f < entry->face_count; f++) {
                    const ModelFaceUvInfo *uv = &entry->face_uvs[f];
                    if (!uv->textured) continue;
                    CHECK(uv->tex_a < entry->base_vert_count && uv->tex_b < entry->base_vert_count &&
                          uv->tex_c < entry->base_vert_count);
                    for (int j = 0; j < 3; j++) {
                        float u = entry->model.meshes[0].texcoords[(f * 3 + j) * 2];
                        float v = entry->model.meshes[0].texcoords[(f * 3 + j) * 2 + 1];
                        CHECK(isfinite(u) && isfinite(v));
                        CHECK(u >= uv->u_base - 0.00001f && u <= uv->u_base + uv->u_scale + 0.00001f);
                        CHECK(v >= uv->v_base - uv->repeat_v_margin * uv->v_scale - 0.00001f &&
                              v <= uv->v_base + (1 + uv->repeat_v_margin) * uv->v_scale + 0.00001f);
                    }
                }
                Camera3D camera = {.position={(kit - 1.5f) * 3.2f, 1.1f + (pose_id - 1) * 3.2f, -7},
                    .target={(kit - 1.5f) * 3.2f, 1.1f + (pose_id - 1) * 3.2f, 0},
                    .up={0, 1, 0}, .fovy=9.6f, .projection=CAMERA_ORTHOGRAPHIC};
                BeginScissorMode(kit * 400, pose_id * 400, 400, 400);
                BeginMode3D(camera);
                /* Start with the old world-pass state, not Raylib's default.
                 * Mode 0 is the live path, 1 reproduces the bug, 2 is RuneC's
                 * explicit one-sided reference. */
                rlDisableBackfaceCulling();
                if (draw_mode == 0) {
                    fc_player_appearance_draw(entry, (Vector3){0, 0, 0}, 0);
                } else {
                    if (draw_mode == 2) {
                        rlSetCullFace(RL_CULL_FACE_BACK);
                        rlEnableBackfaceCulling();
                    }
                    DrawModel(entry->model, (Vector3){0, 0, 0}, 1, WHITE);
                }
                rlEnableBackfaceCulling();
                EndMode3D();
                EndScissorMode();
                anim_model_state_free(pose);
                CHECK(fc_state_hash(&state) == before);
            }
        }
        EndTextureMode();
        Image image = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        char path[64];
        const char *mode = draw_mode == 1 ? "two-sided" : draw_mode == 2 ? "reference" : "appearance";
        snprintf(path, sizeof(path), "cape-%s-%d.png", mode, time);
        CHECK(ExportImage(image, path));
        UnloadImage(image);
    }
    UnloadRenderTexture(target);
    return 1;
}

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
        fc_player_appearance_draw(entry, (Vector3){0, 0, 0}, 180);
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
    ok = check_capes(&appearance, cache, 1) && ok;
    appearance.atlas.anim_ticks = 0;
    ok = check_capes(&appearance, cache, 0) && ok;
    appearance.atlas.anim_ticks = 0;
    ok = check_capes(&appearance, cache, 2) && ok;
    for (int time = 0; time < 2; time++) {
        char path[64];
        snprintf(path, sizeof(path), "cape-appearance-%d.png", time);
        Image actual = LoadImage(path);
        snprintf(path, sizeof(path), "cape-reference-%d.png", time);
        Image reference = LoadImage(path);
        snprintf(path, sizeof(path), "cape-two-sided-%d.png", time);
        Image broken = LoadImage(path);
        if (!actual.data || !reference.data || !broken.data ||
            actual.width != reference.width || actual.height != reference.height ||
            actual.width != broken.width || actual.height != broken.height ||
            actual.format != reference.format || actual.format != broken.format) return 1;
        size_t bytes = (size_t)GetPixelDataSize(actual.width, actual.height, actual.format);
        if (memcmp(actual.data, reference.data, bytes) != 0 ||
            memcmp(actual.data, broken.data, bytes) == 0) {
            fprintf(stderr, "Live cape draw must match one-sided reference, not the old two-sided pass\n");
            return 1;
        }
        UnloadImage(actual);
        UnloadImage(reference);
        UnloadImage(broken);
    }
    anim_cache_free(cache);
    fc_player_appearance_free(&appearance);
    CloseWindow();
    return ok ? 0 : 1;
}
