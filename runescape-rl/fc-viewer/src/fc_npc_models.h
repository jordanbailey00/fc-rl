/*
 * Fight Caves model compatibility wrapper.
 *
 * The viewer historically used NpcModelSet/NpcModelEntry names for every
 * runtime model file. Keep those names while routing all NPC/player/projectile
 * files through the generalized MDL2/MDL3 model loader.
 */

#ifndef FC_NPC_MODELS_H
#define FC_NPC_MODELS_H

#include "fc_models.h"

/* b237 NPC IDs for Fight Caves monsters. */
#define FC_B237_TZ_KIH       3116u
#define FC_B237_TZ_KEK       3118u
#define FC_B237_TZ_KEK_SM    3120u
#define FC_B237_TOK_XIL      3121u
#define FC_B237_YT_MEJKOT    3123u
#define FC_B237_KET_ZEK      3125u
#define FC_B237_TZTOK_JAD    3127u
#define FC_B237_YT_HURKOT    3128u

typedef ModelEntry NpcModelEntry;
typedef ModelSet NpcModelSet;

static uint32_t fc_npc_type_to_model_id(int npc_type) {
    switch (npc_type) {
        case 1: return FC_B237_TZ_KIH;
        case 2: return FC_B237_TZ_KEK;
        case 3: return FC_B237_TZ_KEK_SM;
        case 4: return FC_B237_TOK_XIL;
        case 5: return FC_B237_YT_MEJKOT;
        case 6: return FC_B237_KET_ZEK;
        case 7: return FC_B237_TZTOK_JAD;
        case 8: return FC_B237_YT_HURKOT;
        default: return 0;
    }
}

static NpcModelEntry* fc_npc_model_find(NpcModelSet* set, uint32_t model_id) {
    return model_find(set, model_id);
}

static NpcModelSet* fc_npc_models_load(const char* path,
                                       Texture2D atlas_texture) {
    return models_load(path, atlas_texture);
}

static void fc_npc_models_unload(NpcModelSet* set) {
    models_free(set);
}

#endif /* FC_NPC_MODELS_H */
