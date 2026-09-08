#include "fc_player_appearance.h"
#include "fc_assets.h"
#include "fc_io.h"
#include "fc_items.h"
#include <stdlib.h>
#include <string.h>

int fc_player_appearance_load(FcPlayerAppearance *appearance) {
    memset(appearance, 0, sizeof(*appearance));
    const char *path = "fc_player.parts";
    FILE *file = fc_asset_fopen(path, "rb");
    if (!file) return 0;
    uint32_t header[2];
    int ok = fc_read_exact(file, header, sizeof(uint32_t), 2, path, "header") &&
        header[0] == 0x31504346 && header[1] > 0 && header[1] <= 64;
    if (ok) {
        appearance->record_count = (int)header[1];
        for (int i = 0; i < appearance->record_count; i++) {
            uint32_t row[2];
            if (!fc_read_exact(file, row, sizeof(uint32_t), 2, path, "item mapping") ||
                !fc_item_definition((int)row[0]) || row[1] > 127) { ok = 0; break; }
            appearance->records[i].item_id = row[0];
            appearance->records[i].hide_mask = row[1];
        }
        if (fgetc(file) != EOF) ok = 0;
    }
    fc_asset_close(file);
    if (!ok) { fprintf(stderr, "Invalid player appearance map: %s\n", path); return 0; }
    appearance->parts = models_load("fc_player.models", (Texture2D){0});
    if (!appearance->parts || appearance->parts->has_textures) return 0;
    for (int i = 0; i < 7; i++)
        if (!model_find(appearance->parts, 0xFC100000u + (uint32_t)i)) return 0;
    for (int i = 0; i < appearance->record_count; i++)
        if (!model_find(appearance->parts, appearance->records[i].item_id)) return 0;
    return 1;
}

static ModelSet *compose(ModelEntry *parts[], int count, uint32_t id) {
    int vertices = 0, faces = 0;
    for (int i = 0; i < count; i++) {
        vertices += parts[i]->base_vert_count;
        faces += parts[i]->face_count;
    }
    if (vertices <= 0 || vertices > UINT16_MAX || faces <= 0) return NULL;
    ModelSet *set = calloc(1, sizeof(*set));
    if (!set) return NULL;
    set->entries = calloc(1, sizeof(*set->entries));
    if (!set->entries) { free(set); return NULL; }
    set->count = 1;
    ModelEntry *out = set->entries;
    out->model_id = id;
    out->base_vert_count = vertices;
    out->face_count = faces;
    out->base_verts = malloc((size_t)vertices * 3 * sizeof(int16_t));
    out->vertex_skins = malloc((size_t)vertices);
    out->face_indices = malloc((size_t)faces * 3 * sizeof(uint16_t));
    out->face_priorities = malloc((size_t)faces);
    out->rest_verts = malloc((size_t)faces * 9 * sizeof(float));
    Mesh mesh = {.vertexCount = faces * 3, .triangleCount = faces};
    mesh.vertices = malloc((size_t)faces * 9 * sizeof(float));
    mesh.normals = malloc((size_t)faces * 9 * sizeof(float));
    mesh.colors = malloc((size_t)faces * 12);
    if (!out->base_verts || !out->vertex_skins || !out->face_indices ||
        !out->face_priorities || !out->rest_verts || !mesh.vertices ||
        !mesh.normals || !mesh.colors) {
        free(mesh.vertices); free(mesh.normals); free(mesh.colors);
        models_free(set);
        return NULL;
    }
    int vertex_offset = 0, face_offset = 0;
    for (int i = 0; i < count; i++) {
        const ModelEntry *part = parts[i];
        const Mesh *source = &part->model.meshes[0];
        memcpy(out->base_verts + vertex_offset * 3, part->base_verts,
               (size_t)part->base_vert_count * 3 * sizeof(int16_t));
        memcpy(out->vertex_skins + vertex_offset, part->vertex_skins,
               (size_t)part->base_vert_count);
        for (int f = 0; f < part->face_count * 3; f++)
            out->face_indices[face_offset * 3 + f] =
                (uint16_t)(part->face_indices[f] + vertex_offset);
        memcpy(out->face_priorities + face_offset, part->face_priorities,
               (size_t)part->face_count);
        memcpy(mesh.vertices + face_offset * 9, part->rest_verts,
               (size_t)part->face_count * 9 * sizeof(float));
        memcpy(mesh.normals + face_offset * 9, source->normals,
               (size_t)part->face_count * 9 * sizeof(float));
        memcpy(mesh.colors + face_offset * 12, source->colors,
               (size_t)part->face_count * 12);
        vertex_offset += part->base_vert_count;
        face_offset += part->face_count;
    }
    memcpy(out->rest_verts, mesh.vertices, (size_t)faces * 9 * sizeof(float));
    UploadMesh(&mesh, true);
    out->model = LoadModelFromMesh(mesh);
    out->loaded = set->loaded = 1;
    return set;
}

int fc_player_appearance_sync(FcPlayerAppearance *appearance,
                               const FcPlayer *player, uint32_t model_id) {
    int ids[FC_EQUIPMENT_SLOTS];
    for (int i = 0; i < FC_EQUIPMENT_SLOTS; i++) ids[i] = player->equipment[i].item_id;
    if (appearance->model && appearance->model->entries[0].model_id == model_id &&
        memcmp(ids, appearance->worn_ids, sizeof(ids)) == 0) return 0;
    if (!appearance->parts) return -1;
    ModelEntry *selected[7 + FC_EQUIPMENT_SLOTS];
    int count = 0;
    uint32_t hidden = 0;
    for (int slot = 0; slot < FC_EQUIPMENT_SLOTS; slot++) {
        if (!ids[slot] || slot == FC_EQUIP_SLOT_AMMO || slot == FC_EQUIP_SLOT_RING) continue;
        int record = 0;
        while (record < appearance->record_count &&
               appearance->records[record].item_id != (uint32_t)ids[slot]) record++;
        if (record == appearance->record_count) return -1;
        hidden |= appearance->records[record].hide_mask;
    }
    /* Identity kits precede equipped models, matching client composition. */
    for (int body = 0; body < 7; body++)
        if (!(hidden & (1u << body)))
            selected[count++] = model_find(appearance->parts, 0xFC100000u + (uint32_t)body);
    for (int slot = 0; slot < FC_EQUIPMENT_SLOTS; slot++)
        if (ids[slot] && slot != FC_EQUIP_SLOT_AMMO && slot != FC_EQUIP_SLOT_RING)
            selected[count++] = model_find(appearance->parts, (uint32_t)ids[slot]);
    for (int i = 0; i < count; i++) if (!selected[i]) return -1;
    ModelSet *model = compose(selected, count, model_id);
    if (!model) return -1;
    models_free(appearance->model);
    appearance->model = model;
    memcpy(appearance->worn_ids, ids, sizeof(ids));
    return 1;
}

void fc_player_appearance_free(FcPlayerAppearance *appearance) {
    models_free(appearance->model);
    models_free(appearance->parts);
    memset(appearance, 0, sizeof(*appearance));
}
